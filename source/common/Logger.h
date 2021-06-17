#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <glib.h>

/* log print format
 *
 * ERROR log level has core dump and binary will be stopped. use ERROR level for assertion failure.
 * < ERROR log example. domain name is client >
	2021-06-09T19:37:15.042612 client ERROR    8225  (gint main(gint, gchar**)) TEST 1
	Trace/breakpoint trap
 *
 * < others log example >
	2021-06-09T19:37:31.411759 client CRITICAL 8248  (gint main(gint, gchar**)) TEST 2
	2021-06-09T19:37:31.411853 client WARNING  8248  (gint main(gint, gchar**)) TEST 3
	2021-06-09T19:37:31.411870 client MESSAGE  8248  (gint main(gint, gchar**)) TEST 4 55
	2021-06-09T19:37:31.411956 client INFO     8248  (gint main(gint, gchar**)) TEST 5 hello
	2021-06-09T19:37:31.412031 client DEBUG    8248  (gint main(gint, gchar**)) TEST 6 1 2
	2021-06-09T19:37:31.412069 client CRITICAL 8248  (gint main(gint, gchar**)) TEST 1
	2021-06-09T19:37:31.412134 client CRITICAL     00 01 02 03 04 05 06 07  08 09                    |........ ..      |
	2021-06-09T19:37:31.412202 client WARNING  8248  (gint main(gint, gchar**)) TEST 2
	2021-06-09T19:37:31.412304 client WARNING      00 01 02 03 04 05 06 07  08 09                    |........ ..      |
	2021-06-09T19:37:31.412343 client MESSAGE  8248  (gint main(gint, gchar**)) TEST 3
	2021-06-09T19:37:31.412427 client MESSAGE      00 01 02 03 04 05 06 07  08 09                    |........ ..      |
	2021-06-09T19:37:31.412477 client INFO     8248  (gint main(gint, gchar**)) TEST 4 55
	2021-06-09T19:37:31.412533 client INFO         00 01 02 03 04 05 06 07  08 09                    |........ ..      |
	2021-06-09T19:37:31.412582 client DEBUG    8248  (gint main(gint, gchar**)) TEST 5 hello
	2021-06-09T19:37:31.412631 client DEBUG        00 01 02 03 04 05 06 07  08 09                    |........ ..      |
 */
#define LOG_PRINT(log_level, sub_domain, fmt, args...) g_log (log_get_domain (),          \
                                                            G_PASTE (G_LOG_LEVEL_, log_level),     \
                                                            "%s " fmt, sub_domain, ##args)

#define LOG_ERROR(fmt, args...)            LOG_PRINT (ERROR,    "", fmt, ##args)
#define LOG_CRITICAL(fmt, args...)         LOG_PRINT (CRITICAL, "", fmt, ##args)
#define LOG_WARNING(fmt, args...)          LOG_PRINT (WARNING,  "", fmt, ##args)
#define LOG_MESSAGE(fmt, args...)          LOG_PRINT (MESSAGE,  "", fmt, ##args)
#define LOG_INFO(fmt, args...)             LOG_PRINT (INFO,     "", fmt, ##args)
#define LOG_DEBUG(fmt, args...)            LOG_PRINT (DEBUG,    "", fmt, ##args)

#define LOG_HEX_DUMP_PRINT(log_level, data, len, fmt, args...) {                                                            \
                                                                 G_PASTE (LOG_, log_level) (fmt, ##args);                   \
                                                                 log_hexdump (log_get_domain(),           \
                                                                                         G_PASTE (G_LOG_LEVEL_, log_level), \
                                                                                         data,                              \
                                                                                         len);                              \
                                                               }
#define LOG_HEX_DUMP_CRITICAL(data, len, fmt, args...)         LOG_HEX_DUMP_PRINT(CRITICAL, data, len, fmt, ##args)
#define LOG_HEX_DUMP_WARNING(data, len, fmt, args...)          LOG_HEX_DUMP_PRINT(WARNING,  data, len, fmt, ##args)
#define LOG_HEX_DUMP_MESSAGE(data, len, fmt, args...)          LOG_HEX_DUMP_PRINT(MESSAGE,  data, len, fmt, ##args)
#define LOG_HEX_DUMP_INFO(data, len, fmt, args...)             LOG_HEX_DUMP_PRINT(INFO,     data, len, fmt, ##args)
#define LOG_HEX_DUMP_DEBUG(data, len, fmt, args...)            LOG_HEX_DUMP_PRINT(DEBUG,    data, len, fmt, ##args)

gboolean log_enable     (const gchar *log_domain);
gint     log_get_pid    (void);
gchar   *log_get_domain (void);
void     log_hexdump    (const gchar *log_domain, GLogLevelFlags log_level, const void *data, gint count);
void     log_disable    (void);
#endif
