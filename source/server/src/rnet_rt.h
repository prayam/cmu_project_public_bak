//
// Created by zhou on 18-5-4.
//

#ifndef MAIN_RNET_RT_H
#define MAIN_RNET_RT_H

#include "baseEngine.h"
#include "network.h"
#include <glib.h>


class Rnet_engine : public baseEngine {

	public:
		Rnet_engine();
		~Rnet_engine();
		void init(gint row, gint col) override;
		friend class Rnet;

};

class Rnet {
	public:
		explicit Rnet(const Rnet_engine &rnet_engine);
		~Rnet();
		void run(const cv::Mat &image, const Rnet_engine &engine);
		mydataFmt Rthreshold;
		cudaStream_t stream;
		struct pBox *location_;
		struct pBox *score_;
		struct pBox *rgb;
	private:
		const gint BatchSize;
		const gint INPUT_C;
		const ICudaEngine &Engine;
		//must be computed at runtime
		gint INPUT_H;
		gint INPUT_W;
		gint OUT_PROB_SIZE;
		gint OUT_LOCATION_SIZE;
		gint inputIndex,outputProb,outputLocation;
		void *buffers[3];

};


#endif //MAIN_RNET_RT_H
