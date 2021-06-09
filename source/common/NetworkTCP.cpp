//------------------------------------------------------------------------------------------------
// File: NetworkTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive TCP byte streams for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#include <iostream>
#include <new>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "NetworkTCP.h"
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

	TcpListenPort= g_new0(TTcpListenPort, 1);

	if (TcpListenPort==NULL)
	{
		fprintf(stderr, "TUdpPort memory allocation failed\n");
		return(NULL);
	}
	TcpListenPort->ListenFd=BAD_SOCKET_FD;
#if  defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;
	int     iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		 TcpListenPort;
		printf("WSAStartup failed: %d\n", iResult);
		return(NULL);
	}
#endif

	// create a socket
	if ((TcpListenPort->ListenFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("socket failed");
		return(NULL);
	}
	int option = 1;

	if(setsockopt(TcpListenPort->ListenFd,SOL_SOCKET,SO_REUSEADDR,(char*)&option,sizeof(option)) < 0)
	{
		CloseTcpListenPort(&TcpListenPort);
		perror("setsockopt failed");
		return(NULL);
	}

	// bind it to all local addresses and pick any port number
	memset((char *)&myaddr, 0, sizeof(myaddr));
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
	char *line;
	cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
	if ( cert != NULL )
	{
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

static SSL_CTX *get_server_context(const char *ca_pem,
		const char *cert_pem,
		const char *key_pem) {
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
		const char *ca_pem, const char *cert_pem, const char *key_pem)
{
	TTcpConnectedPort *TcpConnectedPort;
	gboolean isSsl = false;
	int rc = -1;

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

	int bufsize = 200 * 1024;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET,
				SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
		return(NULL);
	}
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET,
				SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
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

static SSL_CTX *get_client_context(const char *ca_pem,
		const char *cert_pem,
		const char *key_pem) {
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
TTcpConnectedPort *OpenTcpConnection(const char *remotehostname, const char * remoteportno,
		const char *ca_pem, const char *cert_pem, const char *key_pem)
{
	TTcpConnectedPort *TcpConnectedPort;
	struct sockaddr_in myaddr;
	int                s;
	struct addrinfo   hints;
	struct addrinfo   *result = NULL;
	gboolean isSsl = false;
	BIO *sbio;
	SSL *ssl;
	SSL_CTX *ctx;
	X509 *server_cert;

	TcpConnectedPort= g_new0(TTcpConnectedPort, 1);

	if (TcpConnectedPort==NULL)
	{
		fprintf(stderr, "TUdpPort memory allocation failed\n");
		return(NULL);
	}
	TcpConnectedPort->ConnectedFd=BAD_SOCKET_FD;
#if  defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;ssl
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		g_free(TcpConnectedPort);
		printf("WSAStartup failed: %d\n", iResult);
		return(NULL);
	}
#endif
	// create a socket
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	s = getaddrinfo(remotehostname, remoteportno, &hints, &result);
	if (s != 0)
	{
		g_free(TcpConnectedPort);
		fprintf(stderr, "getaddrinfo: Failed\n");
		return(NULL);
	}
	if ( result==NULL)
	{
		g_free(TcpConnectedPort);
		fprintf(stderr, "getaddrinfo: Failed\n");
		return(NULL);
	}
	if ((TcpConnectedPort->ConnectedFd= socket(AF_INET, SOCK_STREAM, 0)) == BAD_SOCKET_FD)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		freeaddrinfo(result);
		perror("socket failed");
		return(NULL);
	}

	int bufsize = 200 * 1024;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET,
				SO_SNDBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
		return(NULL);
	}
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET,
				SO_RCVBUF, (char *)&bufsize, sizeof(bufsize)) == -1)
	{
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_SNDBUF failed");
		return(NULL);
	}

// recv timeout set
#ifdef G_OS_WIN32
	// WINDOWS
	DWORD timeout = 5 * 1000;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout) == -1) {
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_RCVTIMEO failed");
		return(NULL);
	}
#else
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(TcpConnectedPort->ConnectedFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) == -1) {
		CloseTcpConnectedPort(&TcpConnectedPort);
		perror("setsockopt SO_RCVTIMEO failed");
		return(NULL);
	}
#endif

	if(ca_pem != NULL && cert_pem != NULL && key_pem != NULL) {
		isSsl = true;
	}

	if (isSsl) {
		/* Initialize OpenSSL */
		SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();

		/* Get a context */
		if (!(ctx = get_client_context(ca_pem, cert_pem, key_pem))) {
			return (NULL);
		}

		/* Get a BIO */
		if (!(sbio = BIO_new_ssl_connect(ctx))) {
			fprintf(stderr, "Could not get a BIO object from context\n");
			goto fail1;
		}

		/* Get the SSL handle from the BIO */
		BIO_get_ssl(sbio, &ssl);

		/* set non blocking IO */
		// BIO_set_nbio(sbio, 1);

		gchar conn_str[128];
		g_snprintf(conn_str, sizeof(conn_str), "%s:%s", remotehostname, remoteportno);
		/* Connect to the server */
		if (BIO_set_conn_hostname(sbio, conn_str) != 1) {
			fprintf(stderr, "Could not connecto to the server\n");
			goto fail2;
		}

		/* Perform SSL handshake with the server */
		if (SSL_do_handshake(ssl) != 1) {
			fprintf(stderr, "SSL Handshake failed\n");
			goto fail2;
		}

		/* Verify that SSL handshake completed successfully */
		if (SSL_get_verify_result(ssl) != X509_V_OK) {
			fprintf(stderr, "Verification of handshake failed\n");
			goto fail2;
		}

		/* Host name Verification is required!!!*/
		// Recover the server's certificate
		server_cert =  SSL_get_peer_certificate(ssl);
		if (server_cert == NULL) {
			// The handshake was successful although the server did not provide a certificate
			// Most likely using an insecure anonymous cipher suite... get out!
			fprintf(stderr, "SSL_get_peer_certificate failed.\n");
			goto fail3;
		}

		// Validate the hostname
		if (validate_hostname(TARGET_HOST, server_cert) != MatchFound) {
			fprintf(stderr, "Hostname validation failed.\n");
			goto fail_4;
		}

		/* Inform the user that we've successfully connected */
		printf("SSL handshake successful with %s\n", conn_str);
		TcpConnectedPort->isSsl = true;
		TcpConnectedPort->ssl = ssl;
		TcpConnectedPort->ctx = ctx;
	}
	else {
		if (connect(TcpConnectedPort->ConnectedFd,result->ai_addr,result->ai_addrlen) < 0)
		{
			CloseTcpConnectedPort(&TcpConnectedPort);
			freeaddrinfo(result);
			perror("connect failed");
			return(NULL);
		}
	}
	freeaddrinfo(result);
	return(TcpConnectedPort);

	/* Cleanup and exit */
fail_4:
	X509_free(server_cert);
fail3:
	BIO_ssl_shutdown(sbio);
fail2:
	BIO_free_all(sbio);
fail1:
	SSL_CTX_free(ctx);
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
	if ((*TcpConnectedPort) == NULL) {
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
//		- 0 = peer is disconnected
//		- negative = error
//-----------------------------------------------------------------
ssize_t ReadDataTcp(TTcpConnectedPort *TcpConnectedPort, unsigned char *data, size_t length)
{
	ssize_t bytes;

	for (size_t i = 0; i < length; i += bytes)
	{
		if (TcpConnectedPort->isSsl) {
			bytes = SSL_read(TcpConnectedPort->ssl, (char *)(data + i), length - i);
		}
		else {
			bytes = recv(TcpConnectedPort->ConnectedFd, (char *)(data + i), length - i, 0);
		}

		if (bytes <= 0) {
			g_print("some error: %ld. if it's 0, peer is disconnected\n", bytes);
			return bytes;
		}
	}

	return (length);
}
//-----------------------------------------------------------------
// END ReadDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// WriteDataTcp - Writes the specified amount TCP data
//-----------------------------------------------------------------
ssize_t WriteDataTcp(TTcpConnectedPort *TcpConnectedPort,unsigned char *data, size_t length)
{
	ssize_t total_bytes_written = 0;
	ssize_t bytes_written;
	while (total_bytes_written != length)
	{
		if (TcpConnectedPort->isSsl) {
			bytes_written = SSL_write(TcpConnectedPort->ssl,
					(char *)(data+total_bytes_written),
					length - total_bytes_written);
		}
		else {
			bytes_written = send(TcpConnectedPort->ConnectedFd,
					(char *)(data+total_bytes_written),
					length - total_bytes_written,0);

		}

		if (bytes_written == -1)
		{
			return(-1);
		}
		total_bytes_written += bytes_written;
	}
	return(total_bytes_written);
}
//-----------------------------------------------------------------
// END WriteDataTcp
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------


