#ifndef TRT_L2NORM_HELPER_PLUGIN_H
#define TRT_L2NORM_HELPER_PLUGIN_H
#include "NvInferPlugin.h"
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <string>
#include <vector>
#include <glib.h>

#define ASSERT(assertion)                                        \
    {                                                            \
        if (!(assertion))                                        \
        {                                                        \
            std::cout<<"ASSERTION FAILED in "                    \
                     <<__FILE__<<":"<<__LINE__                   \
                     <<std::endl;                                \
            abort();                                             \
        }                                                        \
    }

using namespace std;
using namespace nvinfer1;
using namespace nvinfer1::plugin;

typedef enum
{
  OP_TYPE_MAX=0,
  OP_TYPE_RSQRT=1,
  OP_TYPE_SQRT=2
} operation_t;

class L2NormHelper : public IPluginV2
{
public:
    L2NormHelper(gint op_type, float eps);

    L2NormHelper(gint op_type, float eps, gint C, gint H, gint W);

    L2NormHelper(const void* buffer, gsize length);

    ~L2NormHelper() override = default;

    gint getNbOutputs() const override;

    Dims getOutputDimensions(gint index, const Dims* inputs, gint nbInputDims) override;

    gint initialize() override;

    void terminate() override;

    gsize getWorkspaceSize(gint maxBatchSize) const override;

    gint enqueue(
        gint batchSize, const void* const* inputs, void** outputs, void* workspace, cudaStream_t stream) override;

    gsize getSerializationSize() const override;

    void serialize(void* buffer) const override;

    bool supportsFormat(DataType type, PluginFormat format) const override;

    const gchar* getPluginType() const override;

    const gchar* getPluginVersion() const override;

    void destroy() override;

    IPluginV2* clone() const;

    void setPluginNamespace(const gchar* pluginNamespace) override;

    const gchar* getPluginNamespace() const override;

    void configureWithFormat(
        const Dims* inputDims, gint nbInputs, const Dims* outputDims, gint nbOutputs,
        DataType type, PluginFormat format, gint maxBatchSize) override;

private:
    gint C, H, W;
    gint op_type;
    float eps = 1e-12;
    DataType mDataType{DataType::kHALF};
    const gchar* mPluginNamespace;
};

class L2NormHelperPluginCreator : public IPluginCreator
{
public:
    L2NormHelperPluginCreator();

    ~L2NormHelperPluginCreator() override = default;

    const gchar* getPluginName() const override;

    const gchar* getPluginVersion() const override;

    void setPluginNamespace(const gchar* ns) override;

    const gchar* getPluginNamespace() const override;

    const PluginFieldCollection* getFieldNames() override;

    IPluginV2* createPlugin(const gchar* name, const PluginFieldCollection* fc) override;

    IPluginV2* deserializePlugin(const gchar* name, const void* serialData, gsize serialLength) override;

private:
    static PluginFieldCollection mFC;
    gint mOpType;
    float mEps;
    static vector<PluginField> mPluginAttributes;

protected:
    string mNamespace;
};

// Write values into buffer
template <typename T>
void write(gchar*& buffer, const T& val)
{
    *reinterpret_cast<T*>(buffer) = val;
    buffer += sizeof(T);
}

// Read values from buffer
template <typename T>
T read(const gchar*& buffer)
{
    T val = *reinterpret_cast<const T*>(buffer);
    buffer += sizeof(T);
    return val;
}

REGISTER_TENSORRT_PLUGIN(L2NormHelperPluginCreator);

#endif // TRT_L2NORM_HELPER_PLUGIN_H
