/*
 *  client.h
 *  OpenSSL
 *
 *  Created by Thirumal Venkat on 18/05/16.
 *  Copyright © 2016 Thirumal Venkat. All rights reserved.
 */

#ifndef client_h
#define client_h

gint client(const gchar *conn_str,
           const gchar *ca_pem,
           const gchar *cert_pem,
           const gchar *key_pem);

#endif /* client_h */
