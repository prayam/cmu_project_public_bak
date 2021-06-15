//------------------------------------------------------------------------------------------------
// File: TcpSendRecvJpeg.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and receive jpeg images
//------------------------------------------------------------------------------------------------
#ifndef TcpSendRecvJpegH
#define TcpSendRecvJpegH

#include <glib.h>
#include <opencv2/core/core.hpp>
#include "NetworkTCP.h"

gint TcpSendImageAsJpeg(TTcpConnectedPort * TcpConnectedPort, cv::Mat Image);
gssize TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort, cv::Mat *Image);

gboolean TcpSendLoginData(TTcpConnectedPort * TcpConnectedPort, const gchar* userid, const gchar* userpw);
gssize TcpRecvLoginData(TTcpConnectedPort * TcpConnectedPort, gchar** userid, gchar** userpw);
gboolean TcpSendLogoutReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendNonSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendTestRunModeReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendRunModeReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendCaptureReq(TTcpConnectedPort * TcpConnectedPort);
gboolean TcpSendSaveReq(TTcpConnectedPort * TcpConnectedPort, const gchar *name);
gssize TcpRecvCtrlReq(TTcpConnectedPort * TcpConnectedPort, gchar *req_id, void **data);
gint TcpSendRes(TTcpConnectedPort * TcpConnectedPort, guchar res);
gssize TcpRecvRes(TTcpConnectedPort * TcpConnectedPort, guchar *res);

gboolean TcpSendMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> meta);
gssize TcpRecvMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> &meta);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
