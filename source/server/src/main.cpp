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
#include "TcpSendRecvJpeg.h"
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

static gint UserAthenticate(gchar **userid, gchar **userpw)
{
	gint ret = 0;

	if (*userid == NULL || *userpw == NULL)
		goto exit;

	if (fileExists("./asset/credential")) {
		std::ifstream file("./asset/credential", std::ios::binary);
		if (file.good())
		{
			gchar buf_id[32];
			gchar buf_pw[32];
			gchar user_pw[MAX_ACCOUNT_PW + 2]; /* 2 is for salt */
			guchar *user_hashed_id;
			guchar *user_hashed_pw;

			file.read(buf_id, 32);
			file.read(buf_pw, 32);
			file.close();

			g_strlcpy(user_pw, *userpw, MAX_ACCOUNT_PW + 2);
			g_strlcat(user_pw, "6^", MAX_ACCOUNT_PW + 2);

			make_sha256_m((guchar *)*userid, strlen(*userid), &user_hashed_id);
			make_sha256_m((guchar *)user_pw, strlen(user_pw), &user_hashed_pw);

			if (memcmp(buf_id, user_hashed_id, 32) == 0 &&
			    memcmp(buf_pw, user_hashed_pw, 32) == 0)
				ret = 1;

			g_free(user_hashed_id);
			g_free(user_hashed_pw);
		}
	}

exit:
	if (*userid)
		g_free(*userid);
	if (*userpw)
		g_free(*userpw);
	*userid = NULL;
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

	if (argc <2)
	{
		fprintf(stderr,"usage %s [port] [filename]\n", argv[0]);
		exit(0);
	}

	if (argc==3) UseCamera=false;


	Logger gLogger = Logger();
	// Register default TRT plugins (e.g. LRelu_TRT)
	if (!initLibNvInferPlugins(&gLogger, "")) { return 1; }

	// USER DEFINED VALUES
	const string uffFile="../facenetModels/facenet.uff";
	const string engineFile="../facenetModels/facenet.engine";
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
	getFilePaths("../imgs", paths);
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

	gint port = atoi(argv[1]);
	if (port != 5000)
	{
		printf("Usage: port should be 5000\n");
		return(-1);
	}


	if  ((TcpListenPort=OpenTcpListenPort(port))==NULL)  // Open TCP Network port
	{
		printf("OpenTcpListenPortFailed\n");
		return(-1);
	}


	clilen = sizeof(cli_addr);

	printf("Listening for connections\n");

	while (1) {
		videoStreamer = UseCamera ? videoStreamer_c : videoStreamer_v;

		if (check_server_cert()) {
			printf("Cert error\n");
			return(-1);
		}
		/* 1. Establish control channel */
		if  ((TcpConnectedPort_control=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						getFilepath_ca_cert(),
						getFilepath_server_cert(),
						getFilepath_server_key()))==NULL)
		{
			printf("AcceptTcpConnection Failed\n");
			return(-1);
		}
		printf("Accepted control channel connection Request\n");

		gchar *userid = NULL;
		gchar *userpw = NULL;
		if (TcpRecvLoginData(TcpConnectedPort_control,&userid,&userpw) <= 0) { /* Timeout or error */
			CloseTcpConnectedPort(&TcpConnectedPort_control);
			continue;
		}
		if (!UserAthenticate(&userid, &userpw)) {
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
			printf("AcceptTcpConnection Failed\n");
			return(-1);
		}
		printf("Accepted secure data channel connection Request\n");

		/* 3. Establish non-secure channel */
		if  ((TcpConnectedPort_nsdata=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						NULL,
						NULL,
						NULL))==NULL)
		{
			printf("AcceptTcpConnection Failed\n");
			return(-1);
		}
		printf("Accepted non-secure data channel connection Request\n");

		/* 4. Establish meta channel */
		if  ((TcpConnectedPort_meta=AcceptTcpConnection(TcpListenPort,&cli_addr,&clilen,
						getFilepath_ca_cert(),
						getFilepath_server_cert(),
						getFilepath_server_key()))==NULL)
		{
			printf("AcceptTcpConnection Failed\n");
			return(-1);
		}
		printf("Accepted meta channel connection Request\n");

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
		videoStreamer = videoStreamer_c;

		auto globalTimeEnd = chrono::steady_clock::now();

		auto milliseconds = chrono::duration_cast<chrono::milliseconds>(globalTimeEnd-globalTimeStart).count();
		double seconds = double(milliseconds)/1000.;
		double fps = nbFrames/seconds;

		std::cout << "Counted " << nbFrames << " frames in " << double(milliseconds)/1000. << " seconds!" <<
			" This equals " << fps << "fps.\n";

	}
	videoStreamer_c->release();
	videoStreamer_v->release();
	videoStreamer = NULL;
	log_disable();
	return 0;
}

