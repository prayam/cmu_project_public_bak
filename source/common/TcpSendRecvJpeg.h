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

bool TcpReceiveLoginData(TTcpConnectedPort * TcpConnectedPort,std::string &userid,std::string &userpw);
int TcpSendLoginRes(TTcpConnectedPort * TcpConnectedPort, int res);
int TcpSendMeta(TTcpConnectedPort * TcpConnectedPort, std::vector<struct APP_meta> meta);

#endif
//------------------------------------------------------------------------------------------------
//END of Include
//------------------------------------------------------------------------------------------------
