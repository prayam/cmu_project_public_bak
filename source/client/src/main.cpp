//------------------------------------------------------------------------------------------------
// File: RecvImageTCP.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// This program receives a jpeg image via a TCP Stream and displays it.
//----------------------------------------------------------------------------------------------
#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#ifdef G_OS_WIN32
#else
#include <glib-unix.h> /* for g_unix_signal_add() */
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"

using namespace cv;
using namespace std;


enum CLIENT_STATE {
	CLIENT_STATE_NEED_INPUT_LOGIN = 0,
	CLIENT_STATE_NEED_INPUT_PASSWORD,
	CLIENT_STATE_WAIT_USER_AUTHENTICATION,
	CLIENT_STATE_NEED_INPUT_MENU,
	CLIENT_STATE_END
};

struct client_data {
	GMainLoop *mainloop;
	guint16 remote_port;
	gchar *remote_addr;
	enum CLIENT_STATE state;
	gchar *id;
	gchar *password;

	// for old test;
	gchar **argv;
	gint argc;
};

#ifdef _WIN32
#include <termios.h>
#include <conio.h>
#else
#include <stdio.h>
#define clrscr() printf("\e[1;1H\e[2J")
#endif

static struct client_data *client_data;

static gboolean connect_server (gint argc, gchar *argv[])
{
	TTcpConnectedPort *TcpConnectedPort=NULL;
	bool retvalue;

	if (argc !=3)
	{
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	if ((TcpConnectedPort=OpenTcpConnection(argv[1],argv[2],
			"../../custom/keys/ca/ca.crt",
			"../../custom/keys/client/client.crt",
			"../../custom/keys/client/client.key"))==NULL)	// Open UDP Network port
	{
		printf("OpenTcpConnection\n");
		return(-1);
	}

	namedWindow( "Server", WINDOW_AUTOSIZE );// Create a window for display.

	Mat Image;
	do {
		retvalue=TcpRecvImageAsJpeg(TcpConnectedPort,&Image);

		if( retvalue) imshow( "Server", Image ); // If a valid image is received then display it
		else break;

	} while (waitKey(10) != 'q'); // loop until user hits quit

	CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
	return TRUE;
}

static void prompt (enum CLIENT_STATE state)
{
	switch (state) {
		case CLIENT_STATE_NEED_INPUT_LOGIN:
			g_printf("Login ID: ");
			break;
		case CLIENT_STATE_NEED_INPUT_PASSWORD:
			g_printf("Login Password: ");
			break;
		case CLIENT_STATE_NEED_INPUT_MENU:
			g_printf("1. Run Mode\n");
			g_printf("2. Test Run Mode\n");
			g_printf("3. LEarning Mode\n");
			g_printf("4. Secure Mode\n");
			g_printf("5. Non Secure Mode\n");
			g_printf("6. Exit\n");
			g_printf("7. Test old version\n");
			g_printf(" > select one: ");
			break;
		default:
			break;
	}
	fflush(stdout);
}

static gboolean handle_command (GIOChannel *channel, GIOCondition cond, gpointer data)
{
	gchar *str_return;
	gsize length;
	gsize terminator_pos;
	gint64 menu_no = -1;
	GError *error = NULL;
	struct client_data *client_data = (struct client_data *)data;
	struct termios nflags;
	static struct termios oflags;
	gboolean ret = TRUE;

	if (g_io_channel_read_line (channel, &str_return, &length, &terminator_pos, &error) == G_IO_STATUS_ERROR) {
		g_warning ("Something went wrong");
	}

	if (error != NULL) {
		g_printf ("%s\n", error->message);
		g_clear_error(&error);
		exit(1);
	}

	if (length <= 1) {
		// empty string. str_return including the terminating character(s) like newline
		// also length can be 0 in case of Ctrl+D
		prompt(client_data->state);
		goto exit;
	}

	switch (client_data->state) {
		case CLIENT_STATE_NEED_INPUT_LOGIN:
			client_data->id = g_strndup (str_return, length - 1);

			/* disabling echo in terminal - linux only..
			 * see - https://stackoverflow.com/questions/1196418/getting-a-password-in-c-without-using-getpass-3 */
			tcgetattr(fileno(stdin), &oflags);
			nflags = oflags;
			nflags.c_lflag &= ~ECHO;
			nflags.c_lflag |= ECHONL;

			if (tcsetattr(STDIN_FILENO, TCSANOW, &nflags) != 0) {
				perror("tcsetattr");
				client_data->state = CLIENT_STATE_NEED_INPUT_LOGIN;
			}
			else {
				client_data->state = CLIENT_STATE_NEED_INPUT_PASSWORD;
				prompt(client_data->state);
			}
			break;
		case CLIENT_STATE_NEED_INPUT_PASSWORD:
			client_data->password = g_strndup (str_return, length - 1);

			/* restore terminal */
			if (tcsetattr(STDIN_FILENO, TCSANOW, &oflags) != 0) {
				perror("tcsetattr");
				client_data->state = CLIENT_STATE_NEED_INPUT_LOGIN;
			}
			else {
				// client_data->state = CLIENT_STATE_WAIT_USER_AUTHENTICATION;

				//:TODO: delete test print code below
				g_printf("id: %s, password: %s\n", client_data->id, client_data->password);

				//:TODO: below source should be printed after finishing the user authentication.
				client_data->state = CLIENT_STATE_NEED_INPUT_MENU;
				prompt(client_data->state);
			}
			break;
		case CLIENT_STATE_NEED_INPUT_MENU:
			clrscr();
			// do each mode
			menu_no = g_ascii_strtoll (str_return, NULL, 10);
			if (menu_no <= 0 || menu_no > 7) { //:TODO: enum.. MAX
				g_warning("validation error: menu_no\n");
				return -1;
			}

			if (menu_no == 6) {
				ret = FALSE;
				g_main_loop_quit(client_data->mainloop);
			}
			else if (menu_no == 7) {
				connect_server(client_data->argc, client_data->argv);
				prompt(client_data->state);
			}
			else {
				prompt(client_data->state);
			}
			break;
		default:
			break;
	}

exit:
	g_free (str_return);
	return ret;
}

static gboolean on_signal (gpointer user_data)
{
	int signo = GPOINTER_TO_INT (user_data);

	g_printf("quit mainloop: get signal %s\n", g_strsignal(signo));

	if (client_data != NULL && client_data->mainloop != NULL) {
		g_main_loop_quit(client_data->mainloop);
	}

	return G_SOURCE_REMOVE;
}

gint main(gint argc, gchar *argv[])
{
	if (argc != 3)
	{
		g_warning("usage %s [remote_ipaddr] [remote_port]\n", argv[0]);
		return -1;
	}

	if(!g_hostname_is_ip_address(argv[1])) {
		g_warning("validation error: remote_ipaddr\n");
		return -1;
	}

	gint64 port = 0;
	port = g_ascii_strtoll (argv[2], NULL, 10);
	if (port <= 0 || port > G_MAXUINT16) {
		g_warning("validation error: remote_port\n");
		return -1;
	}

	client_data = g_new0(struct client_data, 1);
	if (client_data == NULL) {
		g_warning("memory allocation fail\n");
		return -1;
	}

	client_data->argc = argc;
	client_data->argv = g_strdupv (argv);
	client_data->remote_addr = g_strdup (argv[1]);
	client_data->remote_port = (guint16) port;
	client_data->mainloop = g_main_loop_new (NULL, FALSE);

	GError *error = NULL;

#ifdef G_OS_WIN32
	GIOChannel *channel = g_io_channel_win32_new_fd (STDIN_FILENO);
#else
	g_unix_signal_add(SIGHUP, on_signal, GINT_TO_POINTER (SIGHUP));
	g_unix_signal_add(SIGINT, on_signal, GINT_TO_POINTER (SIGINT));
	g_unix_signal_add(SIGTERM, on_signal, GINT_TO_POINTER (SIGTERM));

	GIOChannel *channel = g_io_channel_unix_new (STDIN_FILENO);
#endif
	g_io_channel_set_encoding (channel, NULL, &error);
	g_io_add_watch (channel, G_IO_IN, handle_command, client_data);

	client_data->state = CLIENT_STATE_NEED_INPUT_LOGIN;
	prompt(client_data->state);
	g_main_loop_run (client_data->mainloop);

	if (error) {
		g_warning("error: %s\n", error->message);
		g_clear_error (&error);
	}

	g_io_channel_unref (channel);
	g_main_loop_unref (client_data->mainloop);
	g_free (client_data->remote_addr);
	g_free (client_data->id);
	g_free (client_data->password);
	g_strfreev (client_data->argv);
	g_free (client_data);

	return 0;
}
