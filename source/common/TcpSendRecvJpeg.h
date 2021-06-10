//------------------------------------------------------------------------------------------------
// File: TcpSendRecvJpeg.h
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Provides the ability to send and receive jpeg images
//------------------------------------------------------------------------------------------------
#ifndef TcpSendRecvJpegH
#define TcpSendRecvJpegH

#include <opencv2/core/core.hpp>
#include "NetworkTCP.h"

int TcpSendImageAsJpeg(TTcpConnectedPort * TcpConnectedPort, cv::Mat Image);
int TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort, cv::Mat *Image);

bool TcpSendLoginData(TTcpConnectedPort * TcpConnectedPort, const char* userid, const char* userpw);
bool TcpRecvLoginData(TTcpConnectedPort * TcpConnectedPort, char** userid, char** userpw);
bool TcpSendLogoutReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvLogoutReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendNonSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvNonSecureModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendTestRunModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvTestRunModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendRunModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvRunModeReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendCaptureReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpRecvCaptureReq(TTcpConnectedPort * TcpConnectedPort);
bool TcpSendSaveReq(TTcpConnectedPort * TcpConnectedPort, const char *name);
bool TcpRecvSaveReq(TTcpConnectedPort * TcpConnectedPort, char **name);

int TcpSendLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char res);
int TcpRecvLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char *res);
bool TcpSendMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> meta);
bool TcpRecvMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> &meta);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
