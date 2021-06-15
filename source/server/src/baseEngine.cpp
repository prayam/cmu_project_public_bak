//
// Created by zhou on 18-5-4.
//

#include "baseEngine.h"

gint baseEngine::det1_relu_counter = 1;

baseEngine::baseEngine(const gchar * prototxt,const gchar* model,const  gchar* input_name,const gchar*location_name,
        const gchar* prob_name, const gchar *point_name) :
    prototxt(prototxt),
    model(model),
    INPUT_BLOB_NAME(input_name),
    OUTPUT_PROB_NAME(prob_name),
    OUTPUT_LOCATION_NAME(location_name),
    OUTPUT_POINT_NAME(point_name)
{
};

baseEngine::~baseEngine() {
    shutdownProtobufLibrary();
}

void baseEngine::init(gint row,gint col) {
    (void) row;
    (void) col;
}

#define DONOT_USE_DEPRECATED_FUNCTION
void baseEngine::caffeToGIEModel(const std::string &deployFile,                // name for caffe prototxt
    const std::string &modelFile,                // name for model
    const std::vector<std::string> &outputs,   // network outputs
    guint maxBatchSize,                    // batch size - NB must be at least as large as the batch we want to run with)
    IHostMemory *&gieModelStream)    // output buffer for the GIE model
{
    gsize lastIdx = model.find_last_of(".");
    string enginePath = model.substr(0, lastIdx);
	(void) gieModelStream;

    if(enginePath.find("det1_relu") != std::string::npos) {
        enginePath.append(std::to_string(det1_relu_counter));
        enginePath.append(".engine");
        det1_relu_counter++;
    }
    else {
        enginePath.append(".engine");
    }
    std::cout << "rawName = " << enginePath << std::endl;
    if(fileExists(enginePath)) {
        std::vector<gchar> trtModelStream_;
        gsize size{ 0 };

        std::ifstream file(enginePath, std::ios::binary);
        if (file.good())
        {
            file.seekg(0, file.end);
            size = file.tellg();
            file.seekg(0, file.beg);
            trtModelStream_.resize(size);
            std::cout << "size" << trtModelStream_.size() << std::endl;
            file.read(trtModelStream_.data(), size);
            file.close();
        }
        std::cout << "size" << size << std::endl;
        IRuntime* runtime = createInferRuntime(gLogger);
        assert(runtime != nullptr);
        ICudaEngine *engine = runtime->deserializeCudaEngine(trtModelStream_.data(), size, nullptr);
        assert(engine);
        context = engine->createExecutionContext();
        std::cout << std::endl;
    }
    else {
        // create the builder
        IBuilder *builder = createInferBuilder(gLogger);

        // parse the caffe model to populate the network, then set the outputs
#ifndef DONOT_USE_DEPRECATED_FUNCTION
        INetworkDefinition *network = builder->createNetwork();
#else
		INetworkDefinition *network = builder->createNetworkV2(0U);
		IBuilderConfig *config = builder->createBuilderConfig();
#endif
        ICaffeParser *parser = createCaffeParser();

        const IBlobNameToTensor *blobNameToTensor = parser->parse(deployFile.c_str(),
                modelFile.c_str(),
                *network,
                nvinfer1::DataType::kHALF);
        // specify which tensors are outputs
        for (auto &s : outputs)
            network->markOutput(*blobNameToTensor->find(s.c_str()));

        // Build the engine
        builder->setMaxBatchSize(maxBatchSize);
#ifndef DONOT_USE_DEPRECATED_FUNCTION
        builder->setMaxWorkspaceSize(1 << 25);
        ICudaEngine *engine = builder->buildCudaEngine(*network);
#else
		config->setMaxWorkspaceSize(1 << 25);
		ICudaEngine *engine = builder->buildEngineWithConfig(*network, *config);
#endif
        assert(engine);

        context = engine->createExecutionContext();

        // Serialize engine
        ofstream planFile;
        planFile.exceptions(ofstream::failbit | ofstream::badbit);
        try{
            planFile.open(enginePath);
            if (planFile.is_open()) {
                IHostMemory *serializedEngine = engine->serialize();
                planFile.write((gchar *) serializedEngine->data(), serializedEngine->size());
                planFile.close();
            }
        }
        catch(ofstream::failure e) {
            std::cerr << "Exception opening/writing/closing file\n";
        }


        // we don't need the network any more, and we can destroy the parser
        network->destroy();
        parser->destroy();
        builder->destroy();
    }
}
