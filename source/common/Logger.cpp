#include <unistd.h> /* for getpid() */
#include <glib/gprintf.h> /* for g_sprintf() */

#include "Logger.h"

/* ASCII background color code. see https://gist.github.com/radxene/f1e286301763b921baf06074ea46c800 */
#define ANSI_RESET_ALL          "\x1b[0m"

#define ANSI_COLOR_BLACK        "\x1b[30m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_WHITE        "\x1b[37m"

#define ANSI_BACKGROUND_BLACK   "\x1b[40m"
#define ANSI_BACKGROUND_RED     "\x1b[41m"
#define ANSI_BACKGROUND_GREEN   "\x1b[42m"
#define ANSI_BACKGROUND_YELLOW  "\x1b[43m"
#define ANSI_BACKGROUND_BLUE    "\x1b[44m"
#define ANSI_BACKGROUND_MAGENTA "\x1b[45m"
#define ANSI_BACKGROUND_CYAN    "\x1b[46m"
#define ANSI_BACKGROUND_WHITE   "\x1b[47m"

#define ANSI_STYLE_BOLD         "\x1b[1m"
#define ANSI_STYLE_ITALIC       "\x1b[3m"
#define ANSI_STYLE_UNDERLINE    "\x1b[4m"

#define STR_LOG_LEVEL_ERROR     "ERROR   "
#define STR_LOG_LEVEL_CRITICAL  "CRITICAL"
#define STR_LOG_LEVEL_WARNING   "WARNING "
#define STR_LOG_LEVEL_MESSAGE   "MESSAGE "
#define STR_LOG_LEVEL_INFO      "INFO    "
#define STR_LOG_LEVEL_DEBUG     "DEBUG   "
#define STR_LOG_LEVEL_UNKNOWN   "UNKNOWN "

#define STR_LOG_LEVEL_ERROR_WITH_COLOR    ANSI_BACKGROUND_BLACK	ANSI_COLOR_RED	  STR_LOG_LEVEL_ERROR    ANSI_RESET_ALL
#define STR_LOG_LEVEL_CRITICAL_WITH_COLOR ANSI_BACKGROUND_BLACK	ANSI_COLOR_RED	  STR_LOG_LEVEL_CRITICAL ANSI_RESET_ALL
#define STR_LOG_LEVEL_WARNING_WITH_COLOR  ANSI_BACKGROUND_BLACK	ANSI_COLOR_YELLOW STR_LOG_LEVEL_WARNING  ANSI_RESET_ALL
#define STR_LOG_LEVEL_MESSAGE_WITH_COLOR  ANSI_BACKGROUND_BLACK	ANSI_COLOR_GREEN  STR_LOG_LEVEL_MESSAGE  ANSI_RESET_ALL
#define STR_LOG_LEVEL_INFO_WITH_COLOR     ANSI_BACKGROUND_BLACK	ANSI_COLOR_CYAN   STR_LOG_LEVEL_INFO     ANSI_RESET_ALL

static guint    log_handler_id;
static gchar    log_domain[64];
static GPid     pid;

static const gchar *log_level_to_string (guint level)
{
	switch (level) {
		case G_LOG_LEVEL_ERROR:    return STR_LOG_LEVEL_ERROR;
		case G_LOG_LEVEL_CRITICAL: return STR_LOG_LEVEL_CRITICAL;
		case G_LOG_LEVEL_WARNING:  return STR_LOG_LEVEL_WARNING;
		case G_LOG_LEVEL_MESSAGE:  return STR_LOG_LEVEL_MESSAGE;
		case G_LOG_LEVEL_INFO:     return STR_LOG_LEVEL_INFO;
		case G_LOG_LEVEL_DEBUG:    return STR_LOG_LEVEL_DEBUG;
		default:                   return STR_LOG_LEVEL_UNKNOWN;
	}
}

static void log_handler_cb (const gchar    *log_domain,
							GLogLevelFlags  log_level,
							const gchar    *message,
							gpointer        user_data)
{
	(void)user_data;

	/* Ignore debug messages if disabled. */
	if ((log_handler_id == 0 && (log_level & G_LOG_LEVEL_DEBUG))) {
		return;
	}

	GDateTime *dt;

	dt = g_date_time_new_now_local ();
	// time_str sample : 2020-05-11T04:52:06.683346
	g_print ("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.6d %s %s %s\n",
		g_date_time_get_year (dt),
		g_date_time_get_month (dt),
		g_date_time_get_day_of_month (dt),
		g_date_time_get_hour (dt),
		g_date_time_get_minute (dt),
		g_date_time_get_second (dt),
		g_date_time_get_microsecond (dt),
		log_domain, log_level_to_string (log_level & G_LOG_LEVEL_MASK),
		message);
	g_date_time_unref (dt);
}

static gboolean is_log_enabled (void)
{
	return (log_handler_id == 0) ? FALSE : TRUE;
}

gboolean log_enable (const gchar *_log_domain)
{
	gboolean ret = FALSE;

	if (!is_log_enabled ()) {
		log_handler_id = g_log_set_handler (_log_domain, (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), log_handler_cb, NULL);
		g_snprintf (log_domain, sizeof (log_domain), "%s", _log_domain);
		pid = getpid ();
		ret = TRUE;
	}

	return ret;
}

gint log_get_pid (void) {
	return (gint) pid;
}

gchar *log_get_domain (void)
{
	if (is_log_enabled ()) {
		return log_domain;
	}
	else {
		return G_LOG_DOMAIN;
	}
}

void log_hexdump (const gchar *log_domain, GLogLevelFlags log_level, const void *data, gint count)
{
	/* see https://github.com/turon/mantis/blob/master/src/tools/cortex/util/log-hexdump.c */
	gchar hex[60];
	gchar ascii[20];
	gint byte, hex_dst, ascii_dst;

	hex_dst = 0;
	ascii_dst = 0;

	for (byte = 0; byte < count; byte ++) {
		g_sprintf (&hex[hex_dst], "%02X ", ((guchar *)data)[byte]);
		hex_dst += 3;

		if (g_ascii_isprint (((guchar *)data)[byte])) {
			g_sprintf (&ascii[ascii_dst], "%c", ((guchar *)data)[byte]);
		} else {
			g_sprintf (&ascii[ascii_dst], ".");
		}
		ascii_dst++;

		if (byte % 16 == 15) {
			g_log (log_domain, log_level, "    %s |%s|", hex, ascii);
			hex_dst = 0;
			ascii_dst = 0;
		}
		else if (byte % 8 == 7) {
			g_sprintf (&hex[hex_dst], " ");
			hex_dst++;
			g_sprintf (&ascii[ascii_dst], " ");
			ascii_dst++;
		}
	}

	if (byte % 16 != 0) {
		g_log (log_domain, log_level, "    %-49s |%-17s|", hex, ascii);
	}
}

void log_disable (void)
{
	if (is_log_enabled ()) {
		g_log_remove_handler (log_domain, log_handler_id);
		log_domain[0] = 0;
		pid = 0;
	}

	log_handler_id = 0;
}
