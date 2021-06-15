#ifndef VIDEO_INPUT_WRAPPER_VIDEOSTREAMER_H
#define VIDEO_INPUT_WRAPPER_VIDEOSTREAMER_H

#include <iostream>
#include <assert.h>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <glib.h>



class VideoStreamer {
	private:
		gint m_videoWidth;
		gint m_videoHeight;
		gint m_frameRate;
		cv::VideoCapture *m_capture;

	public:
		VideoStreamer(gint nmbrDevice, gint videoWidth, gint videoHeight, gint frameRate, gboolean isCSICam);
		VideoStreamer(std::string filename, gint videoWidth, gint videoHeight);
		~VideoStreamer();
		void setResolutionDevice(gint width, gint height);
		void setResoltionFile(gint width, gint height);
		void assertResolution();
		void getFrame(cv::Mat &frame);
		std::string gstreamer_pipeline (gint capture_width, gint capture_height, gint display_width, gint 	display_height, gint frameRate, gint flip_method=0);
		void release();
};

#endif //VIDEO_INPUT_WRAPPER_VIDEOSTREAMER_H
