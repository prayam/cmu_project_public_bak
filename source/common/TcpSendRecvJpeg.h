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
int TcpRecvImageAsJpeg(TTcpConnectedPort * TcpConnectedPort,cv::Mat *Image);

bool TcpSendLoginData(TTcpConnectedPort * TcpConnectedPort, const char* userid, const char* userpw);
bool TcpRecvLoginData(TTcpConnectedPort * TcpConnectedPort, char** userid, char** userpw);

int TcpSendLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char res);
int TcpRecvLoginRes(TTcpConnectedPort * TcpConnectedPort, unsigned char *res);
bool TcpSendMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> meta);
bool TcpRecvMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> &meta);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
