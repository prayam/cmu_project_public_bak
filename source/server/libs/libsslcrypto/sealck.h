/*
 * sealck.h
 *
 */

#ifndef SRC_SSLCRYPTO_H_
#define SRC_SSLCRYPTO_H_

#include <stddef.h>


/**
 * @brief : get libsslcrypto version number (string)
 * @param : void
 * @return : library version number
 */
const char* get_libsslcrypto_version(void);

/**
 * @brief : Encrypt file contents
 * @param : input file name (plain), output file name(Encrypted)
 * @return : 0 for success
 */
int enc_ssl_fp (const char* ifname, const char* ofname);
/**
 * @brief : Decrypt file contents
 * @param : input file name (Encrypted), output file name(plain original)
 * @return : 0 for success
 */
int dec_ssl_fp (const char* ifname, const char* ofname);


/**
 * @brief : Encrypt memory buffer contents and save to file
 * @param : input data & it's size, output file name (save Encrypted data)
 * @return : 0 for success
 */
int enc_ssl_mf (unsigned char* in, size_t is, const char* ofname);
/**
 * @brief : Decrypt file contents to memory buffer
 * @in  param : input file name (has Encrypted data)
 * @out param : output plain buffer buffer & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
int dec_ssl_fm (const char* ifname, unsigned char** out, size_t* os);


/**
 * @brief : Encrypt memory buffer contents
 * @in  param : input plain data & it's size
 * @out param : Encrypted data & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
int enc_ssl (unsigned char* in, size_t is, unsigned char** out, size_t* os);
/**
 * @brief : Decrypt memory buffer contents
 * @in  param : input Encrypted data & it's size
 * @out param : output plain data buffer & it's size
 * @return : 0 for success
 * @caution : caller must be free(*out) after using
 *            because memory assigned internally using malloc.
 */
int dec_ssl (unsigned char* in, size_t is, unsigned char** out, size_t* os);

/**
 * @brief : calculate hash using sha256
 * @in  param : input file name
 * @out param : output hash value
 * @return : 0 for success
 * @caution : caller must be free(*hash) after using
 *            because memory assigned internally using malloc.
 */
void
make_sha256_f(const char* fname, unsigned char** hash);

/**
 * @brief : calculate hash using sha256
 * @in  param : input data & it's size
 * @out param : output hash value
 * @return : 0 for success
 * @caution : caller must be free(*hash) after using
 *            because memory assigned internally using malloc.
 */
void
make_sha256_m(const unsigned char* in, size_t is, unsigned char** hash);


#endif /* SRC_SSLCRYPTO_H_ */
