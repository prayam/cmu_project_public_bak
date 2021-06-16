//------------------------------------------------------------------------------------------------
// File: NetworkTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive TCP byte streams for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#include <iostream>
#include <new>
#include <stdio.h>
#include <string.h>
#include <netinet/tcp.h> //for TCP_NODELAY and TCP_QUICKACK
#include <netinet/in.h> //for IPPROTO_TCP
#include <fcntl.h>
#include "NetworkTCP.h"
#include "Logger.h"
#include "openssl_hostname_validation.h"

#define TARGET_HOST "face.recog.server.Jetson"

//-----------------------------------------------------------------
// OpenTCPListenPort - Creates a Listen TCP port to accept
// connection requests
//-----------------------------------------------------------------
TTcpListenPort *OpenTcpListenPort(short localport)
{
	TTcpListenPort *TcpListenPort;
	struct sockaddr_in myaddr;

#if  defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	gint     iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		(void) TcpListenPort;
		printf("WSAStartup failed: %d\n", iResult);
		return(NULL);
	}
#endif
	TcpListenPort= g_new0(TTcpListenPort, 1);

	if (TcpListenPort==NULL)
	{
		fprintf(stderr, "TUdpPort memory allocation failed\n");
		return(NULL);
	}
	TcpListenPort->ListenFd=BAD_SOCKET_FD;

	// create a socket
	if ((TcpListenPort->ListenFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("socket failed");
		return(NULL);
	}
	gint option = 1;

	if(setsockopt(TcpListenPort->ListenFd,SOL_SOCKET,SO_REUSEADDR,(gchar*)&option,sizeof(option)) < 0)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("setsockopt failed");
		return(NULL);
	}

	// bind it to all local addresses and pick any port number
	memset((gchar *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(localport);

	if (bind(TcpListenPort->ListenFd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("bind failed");
		return(NULL);
	}


	if (listen(TcpListenPort->ListenFd,5)< 0)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("bind failed");
		return(NULL);
	}
	return(TcpListenPort);
}
//-----------------------------------------------------------------
// END OpenTCPListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpListenPort - Closes the specified TCP listen port
//-----------------------------------------------------------------
void CloseTcpListenPort(TTcpListenPort **TcpListenPort)
{
	if ((*TcpListenPort)==NULL) return;
	if ((*TcpListenPort)->ListenFd!=BAD_SOCKET_FD)
	{
		CLOSE_SOCKET((*TcpListenPort)->ListenFd);
		(*TcpListenPort)->ListenFd=BAD_SOCKET_FD;
	}
	g_free (*TcpListenPort);
	(*TcpListenPort)=NULL;
#if  defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------


/* [START] by jh.ahn */
static void ShowCerts(SSL* ssl)
{
	X509 *cert;
	cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
	if ( cert != NULL )
	{
		gchar *line;
		printf("Server certificates:\n");
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		printf("Subject: %s\n", line);
		free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		printf("Issuer: %s\n", line);
		free(line);
		X509_free(cert);
	}
	else
	{
		printf("No certificates.\n");
	}
}
/* [END] by jh.ahn */

static SSL_CTX *get_server_context(const gchar *ca_pem,
		const gchar *cert_pem,
		const gchar *key_pem) {
	SSL_CTX *ctx;

	/* Get a default context */
	if (!(ctx = SSL_CTX_new(TLS_server_method()))) {
		fprintf(stderr, "SSL_CTX_new failed\n");
		return NULL;
	}

	/* Set the CA file location for the server */
	if (SSL_CTX_load_verify_locations(ctx, ca_pem, NULL) != 1) {
		fprintf(stderr, "Could not set the CA file location\n");
		goto fail;
	}

	/* Load the client's CA file location as well */
	SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(ca_pem));

	/* Set the server's certificate signed by the CA */
	if (SSL_CTX_use_certificate_file(ctx, cert_pem, SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Could not set the server's certificate\n");
		goto fail;
	}

	/* Set the server's key for the above certificate */
	if (SSL_CTX_use_PrivateKey_file(ctx, key_pem, SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Could not set the server's key\n");
		goto fail;
	}

	/* We've loaded both certificate and the key, check if they match */
	if (SSL_CTX_check_private_key(ctx) != 1) {
		fprintf(stderr, "Server's certificate and the key don't match\n");
		goto fail;
	}

	/* We won't handle incomplete read/writes due to renegotiation */
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

	/* Specify that we need to verify the client as well */
	SSL_CTX_set_verify(ctx,
			SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
			NULL);

	/* We accept only certificates signed only by the CA himself */
	SSL_CTX_set_verify_depth(ctx, 1);

	/* Done, return the context */
	return ctx;

fail:
	SSL_CTX_free(ctx);
	return NULL;
}


//-----------------------------------------------------------------
// AcceptTcpConnection -Accepts a TCP Connection request from a
// Listening port
//-----------------------------------------------------------------
TTcpConnectedPort *AcceptTcpConnection(TTcpListenPort *TcpListenPort,
		struct sockaddr_in *cli_addr,socklen_t *clilen,
		const gchar *ca_pem, const gchar *cert_pem, const gchar *key_pem)
{
	TTcpConnectedPort *TcpConnectedPort;
	gboolean isSsl = false;
	gint rc = -1;

	TcpConnectedPort = g_new0(TTcpConnectedPort, 1);

	if (TcpConnectedPort==NULL)
	{
		fprintf(stderr, "TUdpPort memory allocation failed\n");
		return(NULL);
	}

	if(ca_pem != NULL && cert_pem != NULL && key_pem != NULL) {
		isSsl = true;
	}

	if(isSsl) {
		/* Initialize OpenSSL */
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();

		/* Get a server context for our use */
		if (!(TcpConnectedPort->ctx = get_server_context(ca_pem, cert_pem, key_pem))) {
			fprintf(stderr, "get_server_context failed\n");
			return(NULL);
		}
	}

	TcpConnectedPort->ConnectedFd= accept(TcpListenPort->ListenFd,
			(struct sockaddr *) cli_addr,clilen);

	if (TcpConnectedPort->ConnectedFd== BAD_SOCKET_FD)
	{
		perror("ERROR on accept");
		g_free (TcpConnectedPort);
		return NULL;
	}

	gint bufsize = 200 * 1024;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_RCVBUF, (gchar *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
		return(NULL);
	}

	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_SNDBUF, (gchar *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
		return(NULL);
	}

	gint option = 1;
	if(setsockopt(TcpConnectedPort->ConnectedFd, IPPROTO_TCP, TCP_NODELAY, (gchar*)&option, sizeof(option)) < 0)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt failed");
		return(NULL);
	}

	option = 1;
	if(setsockopt(TcpConnectedPort->ConnectedFd, IPPROTO_TCP, TCP_QUICKACK, (gchar*)&option, sizeof(option)) < 0)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt failed");
		return(NULL);
	}

	if(isSsl) {
		/* Get an SSL handle from the context */
		if (!(TcpConnectedPort->ssl = SSL_new(TcpConnectedPort->ctx))) {
			fprintf(stderr, "Could not get an SSL handle from the context\n");
			goto ssl_exit;
		}

		/* Associate the newly accepted connection with this handle */
		SSL_set_fd(TcpConnectedPort->ssl, TcpConnectedPort->ConnectedFd);

		/* Now perform handshake */
		if ((rc = SSL_accept(TcpConnectedPort->ssl)) != 1) {
			fprintf(stderr, "Could not perform SSL handshake\n");
			if (rc != 0) {
				SSL_shutdown(TcpConnectedPort->ssl);
			}
			SSL_free(TcpConnectedPort->ssl);
			goto ssl_exit;
		}

		ShowCerts(TcpConnectedPort->ssl);
		/* Print success connection message on the server */
		printf("SSL handshake successful with %s:%d\n",
				inet_ntoa(cli_addr->sin_addr), ntohs(cli_addr->sin_port));

		TcpConnectedPort->isSsl = isSsl;
	}

	return TcpConnectedPort;

ssl_exit:
	if(isSsl) {
		SSL_CTX_free(TcpConnectedPort->ctx);
	}
	return NULL;
}
//-----------------------------------------------------------------
// END AcceptTcpConnection
//-----------------------------------------------------------------

static SSL_CTX *get_client_context(const gchar *ca_pem,
		const gchar *cert_pem,
		const gchar *key_pem) {
	SSL_CTX *ctx;

	/* Create a generic context */
	// if (!(ctx = SSL_CTX_new(SSLv23_client_method()))) {
	if (!(ctx = SSL_CTX_new(TLS_client_method()))) {
		fprintf(stderr, "Cannot create a client context\n");
		return NULL;
	}

	/* Load the client's CA file location */
	if (SSL_CTX_load_verify_locations(ctx, ca_pem, NULL) != 1) {
		fprintf(stderr, "Cannot load client's CA file\n");
		goto fail;
	}

	/* Load the client's certificate */
	if (SSL_CTX_use_certificate_file(ctx, cert_pem, SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Cannot load client's certificate file\n");
		goto fail;
	}

	/* Load the client's key */
	if (SSL_CTX_use_PrivateKey_file(ctx, key_pem, SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Cannot load client's key file\n");
		goto fail;
	}

	/* Verify that the client's certificate and the key match */
	if (SSL_CTX_check_private_key(ctx) != 1) {
		fprintf(stderr, "Client's certificate and key don't match\n");
		goto fail;
	}

	/* We won't handle incomplete read/writes due to renegotiation */
	SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

	/* Specify that we need to verify the server's certificate */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

	/* We accept only certificates signed only by the CA himself */
	SSL_CTX_set_verify_depth(ctx, 1);

	/* Done, return the context */
	return ctx;

fail:
	SSL_CTX_free(ctx);
	return NULL;
}

//-----------------------------------------------------------------
// OpenTCPConnection - Creates a TCP Connection to a TCP port
// accepting connection requests
//-----------------------------------------------------------------
TTcpConnectedPort *OpenTcpConnection(const gchar *remotehostname, const gchar * remoteportno,
		const gchar *ca_pem, const gchar *cert_pem, const gchar *key_pem)
{
	TTcpConnectedPort *TcpConnectedPort = NULL;
	gint option, sslfd;
	BIO *sbio = NULL;
	SSL *ssl = NULL;
	SSL_CTX *ctx = NULL;
	X509 *server_cert = NULL;
	struct addrinfo *result = NULL;
	struct addrinfo hints;
	struct timeval tv;
	fd_set fdset;
	long arg;

#if  defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	gint iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		g_free(TcpConnectedPort);
		printf("WSAStartup failed: %d\n", iResult);
		return(NULL);
	}
#endif

	TcpConnectedPort = g_new0(TTcpConnectedPort, 1);
	if (TcpConnectedPort == NULL) {
		LOG_WARNING("memory allocation failed");
		goto error;
	}

	TcpConnectedPort->ConnectedFd = socket(AF_INET, SOCK_STREAM, 0);
	if (TcpConnectedPort->ConnectedFd == BAD_SOCKET_FD)
	{
		LOG_WARNING("socket failed");
		goto error;
	}

	option = 200 * 1024;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_SNDBUF, (gchar *)&option, sizeof(option)) == -1)
	{
		LOG_WARNING("setsockopt SO_SNDBUF failed");
		goto error;
	}

	option = 200 * 1024;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_RCVBUF, (gchar *)&option, sizeof(option)) == -1)
	{
		LOG_WARNING("setsockopt SO_RCVBUF failed");
		goto error;
	}

	option = 1;
	if(setsockopt(TcpConnectedPort->ConnectedFd, IPPROTO_TCP, TCP_NODELAY, (gchar*)&option, sizeof(option)) < 0)
	{
		LOG_WARNING("setsockopt TCP_NODELAY failed");
		goto error;
	}

	option = 1;
	if(setsockopt(TcpConnectedPort->ConnectedFd, IPPROTO_TCP, TCP_QUICKACK, (gchar*)&option, sizeof(option)) < 0)
	{
		LOG_WARNING("setsockopt TCP_QUICKACK failed");
		goto error;
	}

	if (ca_pem != NULL && cert_pem != NULL && key_pem != NULL) {
		/* Initialize OpenSSL */
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();

		/* Get a context */
		if (!(ctx = get_client_context(ca_pem, cert_pem, key_pem))) {
			LOG_WARNING("get_client_context failed");
			goto error;
		}

		/* Get a BIO */
		if (!(sbio = BIO_new_ssl_connect(ctx))) {
			LOG_WARNING("BIO_new_ssl_connect failed");
			goto error_new_ssl_connect;
		}

		/* Get the SSL handle from the BIO */
		BIO_get_ssl(sbio, &ssl);

		/* Connect to the server */
		gchar conn_str[128];
		g_snprintf(conn_str, sizeof(conn_str), "%s:%s", remotehostname, remoteportno);
		if (BIO_set_conn_hostname(sbio, conn_str) != 1) {
			LOG_WARNING("Could not connecto to the server");
			goto error_connect;
		}

		/* set non blocking IO */
		BIO_set_nbio(sbio, 1);

		if (BIO_do_connect(sbio) <= 0) {
			if (!BIO_should_retry(sbio)) {
				LOG_WARNING("BIO_do_connect failed");
				goto error_connect;
			}

			if (BIO_get_fd(sbio, &sslfd) < 0) {
				LOG_WARNING("BIO_get_fd failed");
				goto error_connect;
			}

			FD_ZERO(&fdset);
			FD_SET(sslfd, &fdset);
			tv.tv_sec = 2;  /* 2 second timeout */
			tv.tv_usec = 0;

			if (select(sslfd + 1, NULL, &fdset, NULL, &tv) == 1) { // check write fds
				gint so_error = 0;
				socklen_t len = sizeof(so_error);

				getsockopt(sslfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

				if (so_error != 0) {
					LOG_WARNING("connection error");
					goto error_connect;
				}
			}
			else {
				LOG_WARNING("connection timeout or error");
				goto error_connect;
			}
		}

		g_test_timer_start ();
		while (1) {
			if (SSL_do_handshake(ssl) == 1) {
				break;
			}

			if (g_test_timer_elapsed () > 2.0f) {
				LOG_WARNING("SSL Handshake failed");
				goto error_connect;
			}

			g_usleep(10);
		}

		// reset fd to blocking again. if do not this, recv() is non blocking
		if((arg = fcntl(sslfd, F_GETFL, NULL)) < 0) {
			LOG_WARNING("fcntl block get failed");
			goto error;
		}

		arg &= (~O_NONBLOCK);
		if(fcntl(sslfd, F_SETFL, arg) < 0) {
			LOG_WARNING("fcntl block set failed");
			goto error;
		}

		BIO_set_nbio(sbio, 0);

		/* Verify that SSL handshake completed successfully */
		if (SSL_get_verify_result(ssl) != X509_V_OK) {
			LOG_WARNING("Verification of handshake failed");
			goto error_connect;
		}

		/* Host name Verification is required!!!*/
		// Recover the server's certificate
		server_cert =  SSL_get_peer_certificate(ssl);
		if (server_cert == NULL) {
			// The handshake was successful although the server did not provide a certificate
			// Most likely using an insecure anonymous cipher suite... get out!
			LOG_WARNING("SSL_get_peer_certificate failed");
			goto error_verify_cert;
		}

		// Validate the hostname
		if (validate_hostname(TARGET_HOST, server_cert) != MatchFound) {
			LOG_WARNING("Hostname validation failed");
			goto error_verify_hostname;
		}

		/* Inform the user that we've successfully connected */
		LOG_INFO("SSL handshake successful with %s", conn_str);
		TcpConnectedPort->isSsl = true;
		TcpConnectedPort->ssl = ssl;
		TcpConnectedPort->ctx = ctx;
	}
	else {
		// Set non-blocking
		if( (arg = fcntl(TcpConnectedPort->ConnectedFd, F_GETFL, NULL)) < 0) {
			LOG_WARNING("fcntl nonblock get failed");
			goto error;
		}

		arg |= O_NONBLOCK;
		if( fcntl(TcpConnectedPort->ConnectedFd, F_SETFL, arg) < 0) {
			LOG_WARNING("fcntl nonblock set failed");
			goto error;
		}

		// set addr info to connect
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(remotehostname, remoteportno, &hints, &result) != 0) {
			LOG_WARNING("getaddrinfo failed");
			goto error;
		}

		if (result == NULL) {
			LOG_WARNING("getaddrinfo result is NULL");
			goto error;
		}

		if (connect(TcpConnectedPort->ConnectedFd, result->ai_addr, result->ai_addrlen) < 0)
		{
			FD_ZERO(&fdset);
			FD_SET(TcpConnectedPort->ConnectedFd, &fdset);
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			if (select(TcpConnectedPort->ConnectedFd + 1, NULL, &fdset, NULL, &tv) == 1) { // check write fds
				gint so_error = 0;
				socklen_t len = sizeof(so_error);

				getsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_ERROR, &so_error, &len);

				if (so_error != 0) {
					LOG_WARNING("connection error");
					goto error;
				}
			}
			else {
				LOG_WARNING("connection timeout or error");
				goto error;
			}
		}

		if( (arg = fcntl(TcpConnectedPort->ConnectedFd, F_GETFL, NULL)) < 0) {
			LOG_WARNING("fcntl block get failed");
			goto error;
		}

		arg &= (~O_NONBLOCK);
		if( fcntl(TcpConnectedPort->ConnectedFd, F_SETFL, arg) < 0) {
			LOG_WARNING("fcntl block set failed");
			goto error;
		}

		freeaddrinfo(result);

		LOG_INFO("success TCP connection");
	}

	return TcpConnectedPort;

	/* Cleanup and error */
error_verify_hostname:
	X509_free(server_cert);
error_verify_cert:
	BIO_ssl_shutdown(sbio);
error_connect:
	BIO_free_all(sbio);
error_new_ssl_connect:
	SSL_CTX_free(ctx);
error:
	if (result != NULL) {
		freeaddrinfo(result);
	}

	CloseTcpConnectedPort(&TcpConnectedPort);
	return NULL;
}
//-----------------------------------------------------------------
// END OpenTcpConnection
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// CloseTcpConnectedPort - Closes the specified TCP connected port
//-----------------------------------------------------------------
void CloseTcpConnectedPort(TTcpConnectedPort **TcpConnectedPort)
{
	if (TcpConnectedPort == NULL || (*TcpConnectedPort) == NULL) {
		return;
	}

	if ((*TcpConnectedPort)->ConnectedFd != BAD_SOCKET_FD) {
		if ((*TcpConnectedPort)->isSsl) {
			/* SSL Finalize */
			SSL_shutdown((*TcpConnectedPort)->ssl);
			SSL_free((*TcpConnectedPort)->ssl);
			SSL_CTX_free((*TcpConnectedPort)->ctx);
		}

		CLOSE_SOCKET((*TcpConnectedPort)->ConnectedFd);
		(*TcpConnectedPort)->ConnectedFd = BAD_SOCKET_FD;
	}

	g_free (*TcpConnectedPort);
	(*TcpConnectedPort) = NULL;
#if  defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
}
//-----------------------------------------------------------------
// END CloseTcpListenPort
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// ReadDataTcp - Reads the specified amount TCP data
// return
//		- positive = received bytes
//		- 0 = TCP_RECV_PEER_DISCONNECTED
//		- -1 = TCP_RECV_ERROR
//		- -2 = TCP_RECV_TIMEOUT
//-----------------------------------------------------------------
// TCP_RECV_TIMEOUT_VALUE should be microseconds unit G_USEC_PER_SEC = 1,000,000 micro second = 1sec. defined at glib
#define TCP_RECV_TIMEOUT_VALUE G_USEC_PER_SEC
gssize ReadDataTcp(TTcpConnectedPort *TcpConnectedPort, guchar *data, gsize length)
{
	gint ret;
	gint sslfd;
	fd_set fdset;
	struct timeval tv;
	gssize bytes = 0;

	if (length > G_MAXINT) {
		LOG_WARNING("length > G_MAXINT. too long. cannot read it");
		goto exit;
	}

	if (TcpConnectedPort->isSsl) {
		sslfd = SSL_get_fd(TcpConnectedPort->ssl);
		if (sslfd < 0) {
			LOG_WARNING("SSL_get_fd failed");
		}
	}

	for (gsize i = 0; i < length; i += bytes)
	{
		FD_ZERO(&fdset);
		tv.tv_sec = TCP_RECV_TIMEOUT_VALUE / G_USEC_PER_SEC;
		tv.tv_usec = TCP_RECV_TIMEOUT_VALUE % G_USEC_PER_SEC;

		if (TcpConnectedPort->isSsl) {
			FD_SET(sslfd, &fdset);

			ret = select(sslfd + 1, &fdset, NULL, NULL, &tv);
			if (ret < 0) { // select error
				LOG_DEBUG("select error");
				return TCP_RECV_ERROR;
			}
			else if (ret == 0) { // timeout
				LOG_DEBUG("select timeout");
				return TCP_RECV_TIMEOUT;
			}
			else if (FD_ISSET(sslfd, &fdset)) {
				bytes = SSL_read(TcpConnectedPort->ssl, (gchar *)(data + i), length - i);
			}
			else { // unknown error
				LOG_DEBUG("select unknown error");
				return TCP_RECV_ERROR;
			}
		}
		else {
			FD_SET(TcpConnectedPort->ConnectedFd, &fdset);

			ret = select(TcpConnectedPort->ConnectedFd + 1, &fdset, NULL, NULL, &tv);
			if (ret < 0) { // select error
				LOG_DEBUG("select error");
				return TCP_RECV_ERROR;
			}
			else if (ret == 0) { // timeout
				LOG_DEBUG("select timeout");
				return TCP_RECV_TIMEOUT;
			}
			else if (FD_ISSET(TcpConnectedPort->ConnectedFd, &fdset)) {
				bytes = recv(TcpConnectedPort->ConnectedFd, (gchar *)(data + i), length - i, 0);
			}
			else { // unknown error
				LOG_DEBUG("select unknown error");
				return TCP_RECV_ERROR;
			}
		}

		if (bytes == 0) { // recv error
			LOG_WARNING("peer is disconnected");
			return TCP_RECV_PEER_DISCONNECTED;
		}
		else if (bytes < 0) {
			LOG_DEBUG("recv or SSL_read error");
			return TCP_RECV_ERROR;
		}
	}

exit:
	return length;
}
//-----------------------------------------------------------------
// END ReadDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// WriteDataTcp - Writes the specified amount TCP data
//-----------------------------------------------------------------
gssize WriteDataTcp(TTcpConnectedPort *TcpConnectedPort,guchar *data, gsize length)
{
	gssize total_bytes_written = 0;
	gssize bytes_written;

	if (length > G_MAXINT) {
		LOG_WARNING("length > G_MAXINT. too long. cannot write it");
		goto exit;
	}

	while (total_bytes_written != (gssize)length)
	{
		if (TcpConnectedPort->isSsl) {
			bytes_written = SSL_write(TcpConnectedPort->ssl,
					(gchar *)(data+total_bytes_written),
					(gint) length - total_bytes_written);
		}
		else {
			bytes_written = send(TcpConnectedPort->ConnectedFd,
					(gchar *)(data+total_bytes_written),
					length - total_bytes_written,0);

		}

		if (bytes_written == -1)
		{
			return(-1);
		}

		total_bytes_written += bytes_written;
	}

exit:
	return total_bytes_written;
}
//-----------------------------------------------------------------
// END WriteDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------


