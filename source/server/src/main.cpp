#include <iostream>
#include <string>
#include <chrono>
#include <NvInfer.h>
#include <NvInferPlugin.h>
#include <l2norm_helper.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudawarping.hpp>
#include "faceNet.h"
#include "videoStreamer.h"
#include "network.h"
#include "mtcnn.h"
#include "Logger.h"

#include "NetworkTCP.h"
#include "TcpSendRecv.h"
#include "CommonStruct.h"
#include "certcheck.h"
#include "common.h"
#include <termios.h>

#define MAXFACES	8
gint kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

gint readysocket(gint fd)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd + 1, &fds, NULL, NULL, &tv);
}

gint getch()
{
	gint r;
	guchar c;
	if ((r = read(0, &c, sizeof(c))) < 0) {
		return r;
	} else {
		return c;
	}
}
// Uncomment to print timings in milliseconds
// #define LOG_TIMES

using namespace nvinfer1;
using namespace nvuffparser;

static gint FaceAuthencticate(gchar *userid, mtcnn &mtCNN, FaceNetClassifier &faceNet, VideoStreamer *videoStreamer)
{
	gint maxFacesPerScene = MAXFACES;
	std::vector<struct APP_meta> meta;
	meta.reserve(maxFacesPerScene);
	cv::Mat frame;
	cv::cuda::GpuMat src_gpu, dst_gpu;
	std::vector<struct Bbox> outputBbox;
	outputBbox.reserve(maxFacesPerScene);
	auto starttime = chrono::steady_clock::now();
	auto endtime = chrono::steady_clock::now();

	gint cnt = 0;
	while (std::chrono::duration_cast<chrono::milliseconds>(endtime - starttime).count() < 300) {
		videoStreamer->getFrame(frame);
		cnt++;
		endtime = chrono::steady_clock::now();
	}
	LOG_INFO("how many frames dumped out: %d\n", cnt);

	while (std::chrono::duration_cast<chrono::milliseconds>(endtime - starttime).count() < 5000) {
		meta.clear();

		videoStreamer->getFrame(frame);
		if (frame.empty())
			continue;

		/* In case of UseCamera */
		src_gpu.upload(frame);
		cv::cuda::rotate(src_gpu, dst_gpu, src_gpu.size(), 180, src_gpu.size().width, src_gpu.size().height);
		dst_gpu.download(frame);

		outputBbox = mtCNN.findFace(frame);
		faceNet.forward(frame, outputBbox);
		faceNet.featureMatching(frame, meta);
		faceNet.resetVariables();

		outputBbox.clear();
		frame.release();
		if (meta.size() == 1 && !strcmp(meta[0].name, userid))
			return 1;
		endtime = chrono::steady_clock::now();
	}
	return 0;
}

static gint __UserAthenticate(gchar *userid, gchar *userpw)
{
	gint ret = 0;
	guchar* cred = NULL;
	gsize cred_size = 0;

	if (userid == NULL || userpw == NULL)
		goto exit;

	if (dec_ssl_fm("./asset/credential", &cred, &cred_size)) {
		LOG_WARNING("Could not load credential file");
		goto exit;
	} else {
		gchar buf_id[32];
		gchar buf_pw[32];
		gchar user_pw[MAX_ACCOUNT_PW + 2]; /* 2 is for salt */
		guchar *user_hashed_id;
		guchar *user_hashed_pw;

		if (cred_size != 64) {
			LOG_WARNING("length error. credential file");
			goto exit;
		}

		memcpy(buf_id, cred, 32);
		memcpy(buf_pw, cred+32, 32);

		g_strlcpy(user_pw, userpw, MAX_ACCOUNT_PW + 2);
		g_strlcat(user_pw, "6^", MAX_ACCOUNT_PW + 2);

		make_sha256_m((guchar *)userid, strlen(userid), &user_hashed_id);
		make_sha256_m((guchar *)user_pw, strlen(user_pw), &user_hashed_pw);

		if (memcmp(buf_id, user_hashed_id, 32) == 0 &&
			memcmp(buf_pw, user_hashed_pw, 32) == 0)
			ret = 1;

		g_free(cred);
		g_free(user_hashed_id);
		g_free(user_hashed_pw);
	}

exit:
	return ret;
}

static gint UserAthenticate(gchar **userid, gchar **userpw, mtcnn &mtCNN, FaceNetClassifier &faceNet, VideoStreamer *videoStreamer_c)
{
	gint ret = 0;

	if (userid == NULL || userpw == NULL)
		goto exit;

	if (!__UserAthenticate(*userid, *userpw))
		goto exit;

	if (!FaceAuthencticate(*userid, mtCNN, faceNet, videoStreamer_c))
		goto exit;

	ret = 1;

exit:
	if (userid && *userid)
		g_free(*userid);
	if (userpw && *userpw)
		g_free(*userpw);
	if (userid)
		*userid = NULL;
	if (userpw)
		*userpw = NULL;

	if (ret)
		LOG_INFO("Authentication success\n");
	else
		LOG_INFO("Authentication failed\n");

	return ret;
}

gint main(gint argc, gchar *argv[])
{
	gint maxFacesPerScene = MAXFACES;

	TTcpListenPort    *TcpListenPort;
	TTcpConnectedPort *TcpConnectedPort_control;
	TTcpConnectedPort *TcpConnectedPort_sdata;
	TTcpConnectedPort *TcpConnectedPort_nsdata;
	TTcpConnectedPort *TcpConnectedPort_meta;
	const gchar *testmodefile = "./asset/friends640x480.mp4";
	log_enable("server");

	std::vector<struct APP_meta> meta;
	meta.reserve(maxFacesPerScene);
	gint secure_mode = MODE_SECURE;
	//gint run_mode = MODE_RUN; //unused vairable

	struct sockaddr_in cli_addr;
	socklen_t          clilen;
	gboolean               UseCamera=true;

	if (argc != 2)
	{
		LOG_WARNING("usage %s [port]", argv[0]);
		exit(0);
	}

	if (argc==3) UseCamera=false;

	gint64 port_parsed = g_ascii_strtoll(argv[1], NULL, 10);
	if (port_parsed != 5000)
	{
		LOG_WARNING("Usage: port should be 5000");
		return(-1);
	}

	Logger gLogger = Logger();
	// Register default TRT plugins (e.g. LRelu_TRT)
	if (!initLibNvInferPlugins(&gLogger, "")) { return 1; }

	// USER DEFINED VALUES
	const string uffFile="./asset/facenetModels/facenet.uff";
	const string engineFile="./asset/facenetModels/facenet.engine";
	DataType dtype = DataType::kHALF;
	//DataType dtype = DataType::kFLOAT;
	gboolean serializeEngine = true;
	gint batchSize = 1;
	gint nbFrames = 0;
	// gint videoFrameWidth =1280;
	// gint videoFrameHeight =720;
	gint videoFrameWidth = 640;
	gint videoFrameHeight = 480;

	float knownPersonThreshold = 1.;
	gboolean isCSICam = true;

	// init facenet
	FaceNetClassifier faceNet = FaceNetClassifier(gLogger, dtype, uffFile, engineFile, batchSize, serializeEngine,
			knownPersonThreshold, maxFacesPerScene, videoFrameWidth, videoFrameHeight);

	VideoStreamer *videoStreamer;
	VideoStreamer *videoStreamer_c;
	VideoStreamer *videoStreamer_v;

	// init opencv stuff
	videoStreamer_c = new VideoStreamer(0, videoFrameWidth, videoFrameHeight, 60, isCSICam);
	videoStreamer_v = new VideoStreamer(testmodefile, videoFrameWidth, videoFrameHeight);



	cv::Mat frame;

	// init mtCNN
	mtcnn mtCNN(videoFrameHeight, videoFrameWidth);

	//init Bbox and allocate memory for "maxFacesPerScene" faces per scene
	std::vector<struct Bbox> outputBbox;
	outputBbox.reserve(maxFacesPerScene);

	// get embeddings of known faces
	std::vector<struct Paths> paths;
	cv::Mat image;
	getFilePaths("./asset/imgs", paths);
	for(gsize i=0; i < paths.size(); i++) {
		std::string rawName;
		gint ret = loadInputImage(paths[i].absPath, image, videoFrameWidth, videoFrameHeight, rawName);
		if (ret)
			continue;
		outputBbox = mtCNN.findFace(image);
		faceNet.forwardAddFace(image, outputBbox, rawName);
		faceNet.resetVariables();
	}
	outputBbox.clear();

	if  ((TcpListenPort=OpenTcpListenPort((short)port_parsed))==NULL)  // Open TCP Network port
	{
		LOG_WARNING("OpenTcpListenPortFailed");
		return(-1);
	}


	clilen = sizeof(cli_addr);

	LOG_INFO("Listening for connections");

	while (1) {
		videoStreamer = UseCamera ? videoStreamer_c : videoStreamer_v;

		if (check_server_cert()) {
			LOG_WARNING("Cert error");
			return(-1);
		}
		/* 1. Establish control channel */
		if  ((TcpConnectedPort_control=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						getFilepath_ca_cert(),
						getFilepath_server_cert(),
						getFilepath_server_key()))==NULL)
		{
			LOG_WARNING("AcceptTcpConnection Failed");
			return(-1);
		}
		LOG_INFO("Accepted control channel connection Request");

		gchar *userid = NULL;
		gchar *userpw = NULL;
		if (TcpRecvLoginData(TcpConnectedPort_control,&userid,&userpw) <= 0) { /* Timeout or error */
			CloseTcpConnectedPort(&TcpConnectedPort_control);
			continue;
		}


		if (!UserAthenticate(&userid, &userpw, mtCNN, faceNet, videoStreamer_c)) {
			if (TcpSendRes(TcpConnectedPort_control, RES_FAIL_AUTH) <= 0) {
				CloseTcpConnectedPort(&TcpConnectedPort_control);
				continue;
			}
			CloseTcpConnectedPort(&TcpConnectedPort_control);
			continue;
		}
		else {
			if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0) {
				CloseTcpConnectedPort(&TcpConnectedPort_control);
				continue;
			}
		}

		/* 2. Establish secure channel */
		if  ((TcpConnectedPort_sdata=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						getFilepath_ca_cert(),
						getFilepath_server_cert(),
						getFilepath_server_key()))==NULL)
		{
			LOG_WARNING("AcceptTcpConnection Failed");
			return(-1);
		}
		LOG_INFO("Accepted secure data channel connection Request\n");

		/* 3. Establish non-secure channel */
		if  ((TcpConnectedPort_nsdata=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						NULL,
						NULL,
						NULL))==NULL)
		{
			LOG_WARNING("AcceptTcpConnection Failed");
			return(-1);
		}
		LOG_INFO("Accepted non-secure data channel connection Request\n");

		/* 4. Establish meta channel */
		if  ((TcpConnectedPort_meta=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						getFilepath_ca_cert(),
						getFilepath_server_cert(),
						getFilepath_server_key()))==NULL)
		{
			LOG_WARNING("AcceptTcpConnection Failed");
			return(-1);
		}
		LOG_INFO("Accepted meta channel connection Request");

		cv::cuda::GpuMat src_gpu, dst_gpu;
		cv::Mat dst_img;
		// loop over frames with inference
		auto globalTimeStart = chrono::steady_clock::now();

		while (true) {
			TTcpConnectedPort *DataPort = secure_mode == MODE_SECURE ? TcpConnectedPort_sdata :
				TcpConnectedPort_nsdata;
			meta.clear();

			videoStreamer->getFrame(frame);
			if (frame.empty()) {
				std::cout << "Empty frame! Exiting...\n Try restarting nvargus-daemon by "
					"doing: sudo systemctl restart nvargus-daemon" << std::endl;
				break;
			}
			// Create a destination to paint the source into.
			dst_img.create(frame.size(), frame.type());

			// Push the images into the GPU
			if (UseCamera)
			{
				src_gpu.upload(frame);
				cv::cuda::rotate(src_gpu, dst_gpu, src_gpu.size(), 180, src_gpu.size().width, src_gpu.size().height);
				dst_gpu.download(frame);
			}

#ifdef LOG_TIMES
			auto startMTCNN = chrono::steady_clock::now();
#endif
			outputBbox = mtCNN.findFace(frame);
#ifdef LOG_TIMES
			auto endMTCNN = chrono::steady_clock::now();
			auto startForward = chrono::steady_clock::now();
#endif
			faceNet.forward(frame, outputBbox);
#ifdef LOG_TIMES
			auto endForward = chrono::steady_clock::now();
			auto startFeatM = chrono::steady_clock::now();
#endif
			faceNet.featureMatching(frame, meta);
#ifdef LOG_TIMES
			auto endFeatM = chrono::steady_clock::now();
#endif
			faceNet.resetVariables();

			if (TcpSendImageAsJpeg(DataPort,frame)<0)  break;
			if (TcpSendMeta(TcpConnectedPort_meta, meta) == FALSE)  break;
			//cv::imshow("VideoSource", frame);
			nbFrames++;
			if (readysocket(TcpConnectedPort_control->ConnectedFd))
			{
				gchar req_id;
				void *req_parsed_data;
				gint ret;
wait_req:
				do {
					ret = TcpRecvCtrlReq(TcpConnectedPort_control,&req_id,&req_parsed_data);
				} while (ret == -2);
				if (ret <= 0) break;

				//if (TcpRecvCtrlReq(TcpConnectedPort_control,&req_id,&req_parsed_data) <= 0) break;
				if (req_id == REQ_LOGOUT) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					break;
				} else if (req_id == REQ_DISCON) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					break;
				} else if (req_id == REQ_SECURE) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					secure_mode = MODE_SECURE;
					goto exit_req;
				} else if (req_id == REQ_NONSECURE) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					secure_mode = MODE_NONSECURE;
					goto exit_req;
				} else if (req_id == REQ_TESTRUN) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					UseCamera=false;
					videoStreamer = videoStreamer_v;
					goto exit_req;
				} else if (req_id == REQ_RUN) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					UseCamera=true;
					videoStreamer = videoStreamer_c;
					goto exit_req;
				} else if (req_id == REQ_CAPTURE) {
					if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
						break;
					goto wait_req;
				} else if (req_id == REQ_SAVE) {
					if (req_parsed_data && meta.size()) {
						if (TcpSendRes(TcpConnectedPort_control, RES_OK) <= 0)
							break;
						faceNet.addNewFace_name(frame, outputBbox, (gchar *)req_parsed_data);
					} else {
						if (TcpSendRes(TcpConnectedPort_control, RES_FAIL_OTHERS) <= 0)
							break;
					}
					goto wait_req;
				} else {
					if (TcpSendRes(TcpConnectedPort_control, RES_FAIL_OTHERS))
						break;
					break;
				}
			}
exit_req:
			outputBbox.clear();
			frame.release();
			if (kbhit())
			{
				// Stores the pressed key in ch
				gint keyboard =  getch();

				if (keyboard == 'q') break;
				else if(keyboard == 'n')
				{

					auto dTimeStart = chrono::steady_clock::now();
					videoStreamer->getFrame(frame);
					// Create a destination to paint the source into.
					dst_img.create(frame.size(), frame.type());

					// Push the images into the GPU
					src_gpu.upload(frame);
					cv::cuda::rotate(src_gpu, dst_gpu, src_gpu.size(), 180, src_gpu.size().width, src_gpu.size().height);
					dst_gpu.download(frame);

					outputBbox = mtCNN.findFace(frame);
					if (TcpSendImageAsJpeg(DataPort,frame)<0)  break;
					if (TcpSendMeta(TcpConnectedPort_meta, meta) == FALSE)  break;
					//cv::imshow("VideoSource", frame);
					faceNet.addNewFace(frame, outputBbox);
					auto dTimeEnd = chrono::steady_clock::now();
					globalTimeStart += (dTimeEnd - dTimeStart);

				}
			}


#ifdef LOG_TIMES
			std::cout << "mtCNN took " << std::chrono::duration_cast<chrono::milliseconds>(endMTCNN - startMTCNN).count() << "ms\n";
			std::cout << "Forward took " << std::chrono::duration_cast<chrono::milliseconds>(endForward - startForward).count() << "ms\n";
			std::cout << "Feature matching took " << std::chrono::duration_cast<chrono::milliseconds>(endFeatM - startFeatM).count() << "ms\n\n";
#endif  // LOG_TIMES
		}

		CloseTcpConnectedPort(&TcpConnectedPort_control);
		CloseTcpConnectedPort(&TcpConnectedPort_sdata);
		CloseTcpConnectedPort(&TcpConnectedPort_nsdata);
		CloseTcpConnectedPort(&TcpConnectedPort_meta);
		secure_mode = MODE_SECURE;
		//run_mode = MODE_RUN; //unused vairable
		UseCamera=true;

		auto globalTimeEnd = chrono::steady_clock::now();

		auto milliseconds = chrono::duration_cast<chrono::milliseconds>(globalTimeEnd-globalTimeStart).count();
		double seconds = double(milliseconds)/1000.;
		double fps = nbFrames/seconds;

		std::cout << "Counted " << nbFrames << " frames in " << double(milliseconds)/1000. << " seconds!" <<
			" This equals " << fps << "fps.\n";

	}
	videoStreamer_c->release();
	videoStreamer_v->release();
	log_disable();
	return 0;
}

