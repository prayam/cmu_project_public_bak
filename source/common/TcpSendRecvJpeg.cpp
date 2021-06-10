//------------------------------------------------------------------------------------------------
// File: TcpSendRecvJpeg.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Send and receives OpenCV Mat Images in a Tcp Stream commpressed as Jpeg images
//------------------------------------------------------------------------------------------------
#include <glib.h>
#include <opencv2/highgui/highgui.hpp>
#include "TcpSendRecvJpeg.h"
#include "CommonStruct.h"
#include "Logger.h"

static  int init_values[2] = { cv::IMWRITE_JPEG_QUALITY,80 }; //default(95) 0-100
static  std::vector<int> param (&init_values[0], &init_values[0]+2);
static  std::vector<uchar> sendbuff;//buffer for coding

//-----------------------------------------------------------------
// TcpSendImageAsJpeg - Sends a Open CV Mat Image commressed as a
// jpeg image in side a TCP Stream on the specified TCP local port
// and Destination. return bytes sent on success and -1 on failure
//-----------------------------------------------------------------
int TcpSendImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat Image)
{
	unsigned int imagesize;
	cv::imencode(".jpg", Image, sendbuff, param);
	imagesize=htonl(sendbuff.size()); // convert image size to network format
	if (WriteDataTcp(TcpConnectedPort,(unsigned char *)&imagesize,sizeof(imagesize))!=sizeof(imagesize))
		return(-1);
	return(WriteDataTcp(TcpConnectedPort,sendbuff.data(), sendbuff.size()));
}

//-----------------------------------------------------------------
// END TcpSendImageAsJpeg
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// TcpRecvImageAsJpeg - Sends a Open CV Mat Image commressed as a
// jpeg image in side a TCP Stream on the specified TCP local port
// returns true on success and false on failure
//-----------------------------------------------------------------
int TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat *Image)
{
	ssize_t ret;
	unsigned int imagesize;
	unsigned char *buff = NULL;	/* receive buffer */

	ret = ReadDataTcp(TcpConnectedPort, (unsigned char *)&imagesize, sizeof(imagesize));
	if (ret != sizeof(imagesize)) {
		LOG_WARNING("fail ReadDataTcp");
		goto exit;
	}

	imagesize = ntohl(imagesize); // convert image size to host format
	buff = (unsigned char*)g_malloc(imagesize);
	if (buff == NULL) {
		ret = -1;
		LOG_WARNING("buffer allocation fail");
		goto exit;
	}

	ret = ReadDataTcp(TcpConnectedPort, buff, imagesize);
	if(ret == imagesize) {
		cv::imdecode(cv::Mat(imagesize, 1, CV_8UC1, buff), cv::IMREAD_COLOR, Image);

		if ((*Image).empty()) {
			ret = -1;
			LOG_WARNING("unknown image");
			goto exit;
		}
	}

exit:
	g_free(buff);
	return (int)ret;
}
//-----------------------------------------------------------------
// END TcpRecvImageAsJpeg
//-----------------------------------------------------------------


bool TcpSendLoginData(TTcpConnectedPort * TcpConnectedPort, const char* userid, const char* userpw)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	guint8 *tmp;
	size_t userid_len, userpw_len, req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	if (userid == NULL || userpw == NULL) {
		LOG_WARNING("userid or userpw is NULL");
		goto exit;
	}

	userid_len = strlen(userid);
	userpw_len = strlen(userpw);
	if (userid_len > MAX_ACCOUNT_ID || userpw_len > MAX_ACCOUNT_PW) {
		LOG_WARNING("expected userid max len(%d), but it's (%ld)", MAX_ACCOUNT_ID, strlen(userid));
		LOG_WARNING("expected userpw max len(%d), but it's (%ld)", MAX_ACCOUNT_PW, strlen(userpw));
		goto exit;
	}

	if (MAX_DATA_LEN < (1 + userid_len + 1 + userpw_len)) {
		LOG_WARNING("expected data max len(%d), but it's (%ld)", MAX_DATA_LEN, (1 + userid_len + 1 + userpw_len));
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_LOGIN;
	tmp = (guint8*)req->data;

	*tmp = (guint8)userid_len;
	tmp++;
	memcpy(tmp, userid, userid_len);
	tmp += userid_len;

	*tmp = (guint8)userpw_len;
	tmp++;
	memcpy(tmp, userpw, userpw_len);
	tmp += userpw_len;

	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

/* CAUTION: Caller should free userid and userpw after using them. */
bool TcpRecvLoginData(TTcpConnectedPort * TcpConnectedPort, char** userid, char** userpw)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	guint8 *tmp;
	guint8 userid_len, userpw_len;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_LOGIN) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_LOGIN);
		goto exit;
	}

	tmp = (guint8*)req->data;
	userid_len = *tmp;
	tmp++;
	if (userid_len > MAX_ACCOUNT_ID) {
		LOG_WARNING("exceeds id len %d. read len %d", MAX_ACCOUNT_ID, userid_len);
		goto exit;
	}
	tmp += userid_len;

	userpw_len = *tmp;
	if (userpw_len > MAX_ACCOUNT_PW) {
		LOG_WARNING("exceeds pw len %d. read len %d", MAX_ACCOUNT_PW, userpw_len);
		goto exit;
	}

	*userid = g_strndup (req->data + 1, userid_len);
	if (*userid == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	*userpw = g_strndup (req->data + 1 + userid_len + 1, userpw_len);
	if (*userpw == NULL) {
		LOG_WARNING("memory allocation fail");
		g_free(*userid);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendLogoutReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_LOGOUT;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvLogoutReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_LOGOUT) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_LOGOUT);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendSecureModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_SECURE;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvSecureModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_SECURE) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_SECURE);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendNonSecureModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_NONSECURE;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvNonSecureModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_NONSECURE) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_NONSECURE);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendTestRunModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_TESTRUN;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvTestRunModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_TESTRUN) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_TESTRUN);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendRunModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_RUN;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvRunModeReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_RUN) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_RUN);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendCaptureReq(TTcpConnectedPort * TcpConnectedPort)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	size_t req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_CAPTURE;

	req_len = sizeof(struct APP_command_req);
	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

bool TcpRecvCaptureReq(TTcpConnectedPort * TcpConnectedPort)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_CAPTURE) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_CAPTURE);
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

bool TcpSendSaveReq(TTcpConnectedPort * TcpConnectedPort, const char *name)
{
	bool ret = FALSE;
	struct APP_command_req *req = NULL;
	guint8 *tmp;
	size_t name_len, req_len, write_ret;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	if (name == NULL) {
		LOG_WARNING("name is NULL");
		goto exit;
	}

	name_len = strlen(name);
	if (name_len > MAX_NAME) {
		LOG_WARNING("expected userpw max len(%d), but it's (%ld)", MAX_NAME, name_len);
		goto exit;
	}

	if (MAX_DATA_LEN < (1 + name_len)) {
		LOG_WARNING("expected data max len(%d), but it's (%ld)", MAX_DATA_LEN, (1 + name_len));
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req->req_id = REQ_SAVE;
	tmp = (guint8*)req->data;

	*tmp = (guint8)name_len;
	tmp++;
	memcpy(tmp, name, name_len);

	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (write_ret != req_len) {
		LOG_WARNING("unexpected write len %ld. it should be %ld", write_ret, req_len);
		goto exit;
	}

	ret = TRUE;
	LOG_HEX_DUMP_DEBUG(req, req_len, "read req. len: %ld", write_ret);

exit:
	g_free(req);
	return ret;
}

/* CAUTION: Caller should free name after using it. */
bool TcpRecvSaveReq(TTcpConnectedPort * TcpConnectedPort, char **name)
{
	ssize_t read_len, req_len;
	struct APP_command_req *req;
	guint8 *tmp;
	guint8 name_len;
	bool ret = FALSE;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	req = g_new0(struct APP_command_req, 1);
	if (req == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	req_len = sizeof(struct APP_command_req);
	read_len = ReadDataTcp(TcpConnectedPort, (unsigned char *)req, req_len);
	if (read_len != req_len) {
		LOG_WARNING("unexpected read len %ld. it should be %ld", read_len, req_len);
		goto exit;
	}

	if (req->req_id != REQ_SAVE) {
		LOG_WARNING("unexpected message id %d. it should be %d", req->req_id, REQ_SAVE);
		goto exit;
	}

	tmp = (guint8*)req->data;
	name_len = *tmp;
	tmp++;
	if (name_len > MAX_NAME) {
		LOG_WARNING("exceeds id len %d. read len %d", MAX_NAME, name_len);
		goto exit;
	}

	*name = g_strndup (req->data + 1, name_len);
	if (*name == NULL) {
		LOG_WARNING("memory allocation fail");
		goto exit;
	}

	LOG_HEX_DUMP_DEBUG(req, req_len, "read req");
	ret = TRUE;

exit:
	g_free(req);
	return ret;
}

int TcpSendLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char res)
{
	return WriteDataTcp(TcpConnectedPort, &res, sizeof(char));
}

int TcpRecvLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char *res)
{
	return ReadDataTcp(TcpConnectedPort, res, sizeof(char));
}

bool TcpSendMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> meta)
{
	size_t write_ret;
	guint8 nr = meta.size();

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)&nr, sizeof(guint8));
	if (write_ret != sizeof(guint8)) {
		LOG_WARNING("Meta/nr: unexpected write len %ld. it should be %ld", write_ret, sizeof(guint8));
		goto exit;
	}

	for (int i = 0; i < nr; i++) {
		struct APP_meta tmp;

		g_strlcpy(tmp.name, meta[i].name, MAX_NAME - 1);
		tmp.x1 = htonl(meta[i].x1);
		tmp.y1 = htonl(meta[i].y1);
		tmp.x2 = htonl(meta[i].x2);
		tmp.y2 = htonl(meta[i].y2);

		write_ret = WriteDataTcp(TcpConnectedPort, (unsigned char *)&tmp, sizeof(struct APP_meta));
		if (write_ret != sizeof(struct APP_meta)) {
			LOG_WARNING("Meta/item: unexpected write len %ld. it should be %ld", write_ret, sizeof(struct APP_meta));
			goto exit;
		}
	}
	return TRUE;
exit:
	return FALSE;
}

bool TcpRecvMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> &meta)
{
	size_t read_ret;
	guint8 nr;

	if (TcpConnectedPort == NULL) {
		LOG_WARNING("TcpConnectedPort is NULL");
		goto exit;
	}

	read_ret = ReadDataTcp(TcpConnectedPort, (unsigned char *)&nr, sizeof(guint8));
	if (read_ret != sizeof(guint8)) {
		LOG_WARNING("Meta/nr: unexpected read len %ld. it should be %ld", read_ret, sizeof(guint8));
		goto exit;
	}

	for (int i = 0; i < nr; i++) {
		struct APP_meta tmp;

		read_ret = ReadDataTcp(TcpConnectedPort, (unsigned char *)&tmp, sizeof(struct APP_meta));
		if (read_ret != sizeof(struct APP_meta)) {
			LOG_WARNING("Meta/item: unexpected read len %ld. it should be %ld", read_ret, sizeof(struct APP_meta));
			goto exit;
		}

		tmp.x1 = ntohl(tmp.x1);
		tmp.y1 = ntohl(tmp.y1);
		tmp.x2 = ntohl(tmp.x2);
		tmp.y2 = ntohl(tmp.y2);

		meta.push_back(tmp);

	}
	return TRUE;
exit:
	return FALSE;
}


//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
