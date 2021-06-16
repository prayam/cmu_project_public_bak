#include <cstring>
#include <iostream>
#include <sstream>

#include "l2norm_helper.h"

using namespace std;
using namespace nvinfer1;

namespace
{
const gchar* L2NORM_HELPER_PLUGIN_VERSION{"1"};
const gchar* L2NORM_HELPER_PLUGIN_NAME{"L2Norm_Helper_TRT"};
} // namespace

PluginFieldCollection L2NormHelperPluginCreator::mFC{};
vector<PluginField> L2NormHelperPluginCreator::mPluginAttributes;

L2NormHelper::L2NormHelper(gint op_type, float eps):
  C(0), H(0), W(0), op_type(op_type), eps(eps), mPluginNamespace("") {}

L2NormHelper::L2NormHelper(gint op_type, float eps, gint C, gint H, gint W):
  C(C), H(H), W(W), op_type(op_type), eps(eps), mPluginNamespace("") {}

L2NormHelper::L2NormHelper(const void* buffer, gsize length):
  mPluginNamespace("")
{
    const gchar *d = reinterpret_cast<const gchar*>(buffer), *a = d;
    op_type = read<gint>(d);
    eps = read<float>(d);
    C = read<gint>(d);
    H = read<gint>(d);
    W = read<gint>(d);
    mDataType = read<DataType>(d);
    ASSERT(d == a + length);
}

gint L2NormHelper::getNbOutputs() const
{
    // Plugin layer has 1 output
    return 1;
}

Dims L2NormHelper::getOutputDimensions(gint index, const Dims* inputs, gint nbInputDims)
{
    ASSERT(nbInputDims == 1);
    ASSERT(index == 0);
    ASSERT(inputs[0].nbDims == 3);
    return Dims3(inputs[0].d[0], inputs[0].d[1], inputs[0].d[2]);
}

gint L2NormHelper::initialize()
{
    return 0;
}

void L2NormHelper::terminate() {}

gsize L2NormHelper::getWorkspaceSize(gint maxBatchSize) const
{
    (void) maxBatchSize;
    return 0;
}

gsize L2NormHelper::getSerializationSize() const
{
    // C, H, W, eps, op_type, mDataType
    return sizeof(gint) * 3 + sizeof(float) + sizeof(gint) + sizeof(mDataType);
}

void L2NormHelper::serialize(void* buffer) const
{
    gchar *d = reinterpret_cast<gchar*>(buffer), *a = d;
    write(d, op_type);
    write(d, eps);
    write(d, C);
    write(d, H);
    write(d, W);
    write(d, mDataType);

    ASSERT(d == a + getSerializationSize());
}

bool L2NormHelper::supportsFormat(DataType type, PluginFormat format) const
{
    return ((type == DataType::kFLOAT || type == DataType::kHALF) &&
            (format == PluginFormat::kNCHW));
}

// Set plugin namespace
void L2NormHelper::setPluginNamespace(const gchar* pluginNamespace)
{
    mPluginNamespace = pluginNamespace;
}

const gchar* L2NormHelper::getPluginNamespace() const
{
    return mPluginNamespace;
}

// Configure the layer with input and output data types.
void L2NormHelper::configureWithFormat(
    const Dims* inputDims, gint nbInputs, const Dims* outputDims, gint nbOutputs,
    DataType type, PluginFormat format, gint maxBatchSize)
{
    (void) maxBatchSize;
    ASSERT(format == PluginFormat::kNCHW);
    ASSERT(type == DataType::kFLOAT || type == DataType::kHALF);
    mDataType = type;
    C = inputDims[0].d[0];
    H = inputDims[0].d[1];
    W = inputDims[0].d[2];
    ASSERT(nbInputs == 1);
    ASSERT(nbOutputs == 1);
    ASSERT(inputDims[0].nbDims >= 1); // number of dimensions of the input tensor must be >=1
    ASSERT(inputDims[0].d[0] == outputDims[0].d[0] &&
           inputDims[0].d[1] == outputDims[0].d[1] &&
           inputDims[0].d[2] == outputDims[0].d[2]);
}

const gchar* L2NormHelper::getPluginType() const
{
    return L2NORM_HELPER_PLUGIN_NAME;
}

const gchar* L2NormHelper::getPluginVersion() const
{
    return L2NORM_HELPER_PLUGIN_VERSION;
}

void L2NormHelper::destroy()
{
    delete this;
}

// Clone the plugin
IPluginV2* L2NormHelper::clone() const
{
    // Create a new instance
    IPluginV2* plugin = new L2NormHelper(op_type, eps);

    // Set the namespace
    plugin->setPluginNamespace(mPluginNamespace);
    return plugin;
}

// PluginCreator
L2NormHelperPluginCreator::L2NormHelperPluginCreator() :
	mOpType(0),
	mEps(0.0f)
{
    mPluginAttributes.emplace_back(PluginField("op_type", nullptr, PluginFieldType::kINT32, 1));
    mPluginAttributes.emplace_back(PluginField("eps", nullptr, PluginFieldType::kFLOAT32, 1));

    mFC.nbFields = mPluginAttributes.size();
    mFC.fields = mPluginAttributes.data();
}

const gchar* L2NormHelperPluginCreator::getPluginName() const
{
    return L2NORM_HELPER_PLUGIN_NAME;
}

const gchar* L2NormHelperPluginCreator::getPluginVersion() const
{
    return L2NORM_HELPER_PLUGIN_VERSION;
}

void L2NormHelperPluginCreator::setPluginNamespace(const gchar* ns)
{
    mNamespace = ns;
}

const gchar* L2NormHelperPluginCreator::getPluginNamespace() const
{
    return mNamespace.c_str();
}

const PluginFieldCollection* L2NormHelperPluginCreator::getFieldNames()
{
    return &mFC;
}

IPluginV2* L2NormHelperPluginCreator::createPlugin(const gchar* name, const PluginFieldCollection* fc)
{
    const PluginField* fields = fc->fields;

    (void) name;

    for (gint i = 0; i < fc->nbFields; ++i)
    {
        const gchar* attrName = fields[i].name;
        if (!strcmp(attrName, "op_type"))
        {
            ASSERT(fields[i].type == PluginFieldType::kINT32);
            mOpType = static_cast<gint>(*(static_cast<const gint*>(fields[i].data)));
        }
        if (!strcmp(attrName, "eps"))
        {
            ASSERT(fields[i].type == PluginFieldType::kFLOAT32);
            mEps = static_cast<float>(*(static_cast<const float*>(fields[i].data)));
        }
    }
    L2NormHelper* obj = new L2NormHelper(mOpType, mEps);
    obj->setPluginNamespace(mNamespace.c_str());
    return obj;
}

IPluginV2* L2NormHelperPluginCreator::deserializePlugin(
  const gchar* name, const void* serialData, gsize serialLength)
{
    // This object will be deleted when the network is destroyed, which will
    // call L2NormHelper::destroy()
    L2NormHelper* obj = new L2NormHelper(serialData, serialLength);

    (void) name;

    obj->setPluginNamespace(mNamespace.c_str());
    return obj;
}
