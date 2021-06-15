//
// Created by zhou on 18-5-4.
//
#include "common.h"
#include <string>
#include <cassert>
#include <sstream>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <cuda_runtime_api.h>
#include "NvInfer.h"
#include "NvCaffeParser.h"
#include <glib.h>
#ifndef MAIN_BASEENGINE_H
#define MAIN_BASEENGINE_H
using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace std;


class baseEngine {
public:
	static gint det1_relu_counter;
	baseEngine(const gchar *prototxt,const gchar*model,const gchar*out_name,
			const gchar*location_name,const gchar*prob_name,const gchar *point_name = NULL);
	virtual ~baseEngine();
	virtual void caffeToGIEModel(const std::string& deployFile,				// name for caffe prototxt
			const std::string& modelFile,				// name for model
			const std::vector<std::string>& outputs,   // network outputs
			guint maxBatchSize,					// batch size - NB must be at least as large as the batch we want to run with)
			IHostMemory *&gieModelStream);             // output buffer for the GIE model
	virtual void init(gint row,gint col);
	friend class Pnet;
	const string prototxt;
	const string model;
	const gchar *INPUT_BLOB_NAME;
	const gchar *OUTPUT_PROB_NAME;
	const gchar *OUTPUT_LOCATION_NAME;
	const gchar *OUTPUT_POINT_NAME;
	Logger gLogger;
	IExecutionContext *context;
};


#endif //MAIN_BASEENGINE_H
