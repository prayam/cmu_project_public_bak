/*
 * certcheck.h
 *
 */

#ifndef SRC_SSLCRYPTO_H_
#define SRC_SSLCRYPTO_H_

#include <stddef.h>

/**
 * @brief : get libcertcheck version number (string)
 * @param : void
 * @return : library version number
 */
const gchar* get_libcertcheck_version(void);

/**
 * @brief : Encrypt file contents
 * @param : input file name (plain), output file name(Encrypted)
 * @return : 0 for success
 */
gint enc_ssl_fp (const gchar* ifname, const gchar* ofname);
/**
 * @brief : Decrypt file contents
 * @param : input file name (Encrypted), output file name(plain original)
 * @return : 0 for success
 */
gint dec_ssl_fp (const gchar* ifname, const gchar* ofname);


/**
 * @brief : Encrypt memory buffer contents and save to file
 * @param : input data & it's size, output file name (save Encrypted data)
 * @return : 0 for success
 */
gint enc_ssl_mf (guchar* in, gsize is, const gchar* ofname);
/**
 * @brief : Decrypt file contents to memory buffer
 * @in  param : input file name (has Encrypted data)
 * @out param : output plain buffer buffer & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
gint dec_ssl_fm (const gchar* ifname, guchar** out, gsize* os);


/**
 * @brief : Encrypt memory buffer contents
 * @in  param : input plain data & it's size
 * @out param : Encrypted data & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
gint enc_ssl (guchar* in, gsize is, guchar** out, gsize* os);
/**
 * @brief : Decrypt memory buffer contents
 * @in  param : input Encrypted data & it's size
 * @out param : output plain data buffer & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
gint dec_ssl (guchar* in, gsize is, guchar** out, gsize* os);

/**
 * @brief : calculate hash using sha256
 * @in  param : input file name
 * @out param : output hash value
 * @return : 0 for success
 * @caution : caller must be free(*hash) after using
 *            because memory assigned internally using malloc.
 */
void
make_sha256_f(const gchar* fname, guchar** hash);

/**
 * @brief : calculate hash using sha256
 * @in  param : input data & it's size
 * @out param : output hash value
 * @return : 0 for success
 * @caution : caller must be free(*hash) after using
 *            because memory assigned internally using malloc.
 */
void
make_sha256_m(const guchar* in, gsize is, guchar** hash);

/**
 * @brief : check client side cert & key file (exist && integrity check)
 * @in  param : void
 * @return : 0 for success,
 *           -100 for ca certfile not exist
 *           -101 for client certfile not exist
 *           -102 for client keyfile not exist
 *           -200 for ca certfile is currupted
 *           -201 for client certfile is currupted
 *           -202 for client keyfile is currupted
 */
gint
check_client_cert(void);

/**
 * @brief : check server side cert & key file (exist && integrity check)
 * @in  param : void
 * @return : 0 for success,
 *           -100 for ca certfile not exist
 *           -101 for server certfile not exist
 *           -102 for server keyfile not exist
 *           -200 for ca certfile is currupted
 *           -201 for server certfile is currupted
 *           -202 for server keyfile is currupted
 */
gint
check_server_cert(void);

/**
 * @brief : return file path
 * @in  param : void
 * @return : return file path
 */
const gchar* getFilepath_ca_cert(void);

const gchar* getFilepath_client_cert(void);
const gchar* getFilepath_client_key(void);

const gchar* getFilepath_server_cert(void);
const gchar* getFilepath_server_key(void);

/**
 * @brief : return string for file name
 * @out param : buf for file name, length must be 33
 * @return : 0 for success
 */
gint getFileName_leng32(gchar* fname);

#endif /* SRC_SSLCRYPTO_H_ */
