//
// Created by zhou on 18-4-30.
//

#include "common.h"


void* safeCudaMalloc(gsize memSize)
{
	void* deviceMem;
	CHECK(cudaMalloc(&deviceMem, memSize));
	if (deviceMem == nullptr)
	{
		std::cerr << "Out of memory" << std::endl;
		exit(1);
	}
	return deviceMem;
}


	std::vector<std::pair<int64_t, nvinfer1::DataType>>
calculateBindingBufferSizes(const nvinfer1::ICudaEngine& engine, gint nbBindings, gint batchSize)
{
	std::vector<std::pair<int64_t, nvinfer1::DataType>> sizes;
	for (gint i = 0; i < nbBindings; ++i)
	{
		nvinfer1::Dims dims = engine.getBindingDimensions(i);
		nvinfer1::DataType dtype = engine.getBindingDataType(i);

		int64_t eltCount = volume(dims) * batchSize;
		sizes.push_back(std::make_pair(eltCount, dtype));
	}

	return sizes;
}


inline int64_t volume(const nvinfer1::Dims& d)
{
	int64_t v = 1;
	for (int64_t i = 0; i < d.nbDims; i++)
		v *= d.d[i];
	return v;
}


void getFilePaths(std::string imagesPath, std::vector<struct Paths>& paths) {
	std::cout << "Parsing Directory: " << imagesPath << std::endl;
	DIR *dir;
	struct dirent *entry;
	if ((dir = opendir (imagesPath.c_str())) != NULL) {
		while ((entry = readdir (dir)) != NULL) {
			std::string readmeCheck(entry->d_name);
			if (entry->d_type != DT_DIR && readmeCheck != "README.md") {
				struct Paths tempPaths;
				tempPaths.fileName = std::string(entry->d_name);
				tempPaths.absPath = imagesPath + "/" + tempPaths.fileName;
				paths.push_back(tempPaths);
			}
		}
		closedir (dir);
	}
}


extern gint load_dec_cvimage(cv::Mat &image, std::string &name, const char *fn);
gint loadInputImage(std::string inputFilePath, cv::Mat& image, gint videoFrameWidth, gint videoFrameHeight, std::string &name) {
	gint ret = load_dec_cvimage(image, name, inputFilePath.c_str());
	if (ret)
		return ret;
	cv::resize(image, image, cv::Size(videoFrameWidth, videoFrameHeight));
	return 0;
}
