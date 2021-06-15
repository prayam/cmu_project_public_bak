/*
 *  server.h
 *  OpenSSL
 *
 *  Created by Thirumal Venkat on 18/05/16.
 *  Copyright © 2016 Thirumal Venkat. All rights reserved.
 */

#ifndef server_h
#define server_h

gint server(const gchar *port_str,
           const gchar *ca_pem,
           const gchar *cert_pem,
           const gchar *key_pem);

#endif /* server_h */
