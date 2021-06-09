#ifndef _COMMONSTRUCT_H_
#define _COMMONSTRUCT_H_

enum {
	MODE_SECURE,
	MODE_NONSECURE,	/* Just switch between secure port and non-secure port. */

	/* Each secure and nonsecure mode can have the following mode independently. */
	MODE_TESTRUN,	/* Data: Video file stream */
	MODE_RUN,	/* Data: Camera stream */
	MODE_CAPTURE,	/* Corresponding to learning mode */
	MODE_SAVE,	/* Corresponding to learning mode */
};

enum {
	REQ_LOGIN,	/* Start to connect socket.
			   Login process:
			      1. Enter ID/PW
			      2. Push login button
			      3. Initiate control channel
			      4. Grant the login (2FA)
			         4-1. Success: Initiate secure/non-secure/meta channel
			         4-1. Fail: Disconnect the control channel
			      info 1. Timeout(5 secs) should be performed.
			      info 2. Never accept any requests afterward. */

	REQ_LOGOUT,	/* Disconnect all sockets. */
	REQ_DISCON = REQ_LOGOUT,

	REQ_SECURE,
	REQ_NONSECURE,	/* Just switch between secure port and non-secure port. */

	/* Each secure and nonsecure mode can have the following mode independently. */
	REQ_TESTRUN,	/* Data: Video file stream */
	REQ_RUN,	/* Data: Camera stream */
	REQ_CAPTURE,	/* Corresponding to learning mode */
	REQ_SAVE,	/* Corresponding to learning mode */
};

/* This is only for the control channel. */
enum {
	RES_OK,
	RES_FAIL_AUTH,
	RES_FAIL_SOCK, /* Except control channel */
	RES_FAIL_MODE,
	RES_FAIL_OTHERS,
};

#define MAX_DATA_LEN		63 /* data + req_id would be power of 2 aligned. */
#define MAX_ACCOUNT_ID		10
#define MAX_ACCOUNT_PW		20
#define MAX_NAME		20

#define TIMEOUT_LOGIN		5 /* unit: sec */
#define TIMEOUT_OTHERS		5 /* unit: sec */

struct __attribute__ ((packed)) APP_command_req {
	char req_id;
	char data[MAX_DATA_LEN];	/* Within the array, consists of set of (len(1byte) + data). */
};

struct __attribute__ ((packed)) APP_command_res {
	char res;
};

struct __attribute__ ((packed)) APP_meta {
	char name[MAX_NAME];
	int x1, y1, x2, y2;
};

#endif
