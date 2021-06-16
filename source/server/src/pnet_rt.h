//
// Created by zhou on 18-4-30.
//

#ifndef MAIN_PNET_RT_H
#define MAIN_PNET_RT_H

#include "network.h"
#include "common.h"
#include "baseEngine.h"
#include <glib.h>
#endif //MAIN_PNET_RT_H
using namespace nvinfer1;
using namespace nvcaffeparser1;

class Pnet_engine:public baseEngine
{

	public:
		Pnet_engine();
		~Pnet_engine();
		void init(gint row,gint col) override;
		friend class Pnet;

};



class Pnet
{
	public:
		Pnet(gint row,gint col,const Pnet_engine& pnet_engine);
		~Pnet();
		void run(const cv::Mat &image, float scale,const Pnet_engine& engine);
		float nms_threshold;
		mydataFmt Pthreshold;
		cudaStream_t stream;

		vector<struct Bbox> boundingBox_;
		vector<orderScore> bboxScore_;
	private:

		const gint BatchSize ;
		const gint INPUT_C ;
		const ICudaEngine &Engine;
		//must be computed at runtime
		gint INPUT_H ;
		gint INPUT_W ;
		gint OUT_PROB_SIZE;
		gint OUT_LOCATION_SIZE;
		gint     inputIndex,
			outputProb,
			outputLocation;
		void *buffers[3];
		struct pBox *location_;
		struct pBox *score_;
		struct pBox *rgb;

		void generateBbox(const struct pBox *score, const struct pBox *location, mydataFmt scale);
};

