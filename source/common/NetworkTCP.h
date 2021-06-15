//------------------------------------------------------------------------------------------------
// File: NetworkTCP.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and recvive UDP Packets for both Window and linux platforms
//------------------------------------------------------------------------------------------------
#ifndef NetworkTCPH
#define NetworkTCPH

#if  defined(_WIN32) || defined(_WIN64)
#pragma comment (lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <BaseTsd.h>
#define  CLOSE_SOCKET closesocket
#define  SOCKET_FD_TYPE SOCKET
#define  BAD_SOCKET_FD INVALID_SOCKET
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <unistd.h>
#define  CLOSE_SOCKET close
#define  SOCKET_FD_TYPE gint
#define  BAD_SOCKET_FD  -1
#endif

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <netinet/in.h>

#include <glib.h>

//------------------------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------------------------

typedef struct
{
	SOCKET_FD_TYPE ListenFd;
} TTcpListenPort;

typedef struct
{
	SOCKET_FD_TYPE ConnectedFd;
	gboolean isSsl;
	SSL *ssl;
	SSL_CTX *ctx;
} TTcpConnectedPort;

enum {
	TCP_RECV_TIMEOUT = -2,
	TCP_RECV_ERROR = -1,
	TCP_RECV_PEER_DISCONNECTED = 0
};

//------------------------------------------------------------------------------------------------
//  Function Prototypes
//------------------------------------------------------------------------------------------------
TTcpListenPort *OpenTcpListenPort(short localport);
void CloseTcpListenPort(TTcpListenPort **TcpListenPort);
TTcpConnectedPort *AcceptTcpConnection(TTcpListenPort *TcpListenPort,
		struct sockaddr_in *cli_addr,socklen_t *clilen,
		const gchar *ca_pem, const gchar *cert_pem, const gchar *key_pem);
TTcpConnectedPort *OpenTcpConnection(const gchar *remotehostname, const gchar * remoteportno,
		const gchar *ca_pem, const gchar *cert_pem, const gchar *key_pem);
void CloseTcpConnectedPort(TTcpConnectedPort **TcpConnectedPort);
gssize ReadDataTcp(TTcpConnectedPort *TcpConnectedPort,guchar *data, gsize length);
gssize WriteDataTcp(TTcpConnectedPort *TcpConnectedPort,guchar *data, gsize length);
#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------




