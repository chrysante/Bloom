#include "Bloom/Graphics/Material/MaterialInstance.h"

#include <cstring>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/GPU/HardwareDevice.h"

using namespace bloom;
using namespace ranges::views;

namespace {

struct ParamHash {
    size_t operator()(MaterialPrimitiveParameter const& param) const {
        return utl::hash_combine(param.name, param.dataType);
    }
};

struct ParamEqual {
    bool operator()(MaterialPrimitiveParameter const& a,
                    MaterialPrimitiveParameter const& b) const {
        return a.dataType == b.dataType && a.name == b.name;
    }
};

} // namespace

static void amendParameters(
    MaterialInstance& inst,
    std::vector<MaterialPrimitiveParameter>& existingParams,
    std::vector<MaterialPrimitiveParameter> const& newParams) {
    using Set = utl::hashset<MaterialPrimitiveParameter, ParamHash, ParamEqual>;
    auto set = existingParams | ranges::to<Set>;
    existingParams = newParams;
    for (auto& param: existingParams) {
        auto itr = set.find(param);
        if (itr != set.end()) {
            param.defaultValue = itr->defaultValue;
        }
        inst.setParameter(param.defaultValue.data(), param.bufferOffset,
                          layoutSize(param.dataType));
    }
}

void MaterialInstance::setMaterial(Reference<Material> material) {
    if (material) {
        cpuBuffer.resize(material->paramBufferSize);
        amendParameters(*this, primitiveParameters,
                        material->primitiveParameters);
        textureParameters = material->textureParameters;
        material->instances.insert(weak_from_this());
    }
    mMaterial = std::move(material);
}

void MaterialInstance::setParameter(void* data, size_t offset, size_t size) {
    BL_ASSERT(cpuBuffer.size() >= offset + size);
    std::memcpy(cpuBuffer.data() + offset, data, size);
    mDirty = true;
}

void MaterialInstance::uploadData(HardwareDevice& device) {
    if (!mDirty) {
        return;
    }
    mDirty = false;
    if (gpuBuffer.size() < cpuBuffer.size()) {
        gpuBuffer = device.createBuffer(
            { .size = cpuBuffer.size(), .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(gpuBuffer, cpuBuffer.data(), cpuBuffer.size());
}

YAML::Node MaterialInstance::serialize() const {
    YAML::Node root;
    root["Material"] = mMaterial ? mMaterial->handle() : AssetHandle{};
    root["PrimitiveParameters"] = primitiveParameters;
    root["TextureParameters"] = textureParameters;
    return root;
}

void MaterialInstance::deserialize(YAML::Node const& root,
                                   AssetManager& assetManager) {
    auto matHandle = root["Material"].as<AssetHandle>();
    primitiveParameters = root["PrimitiveParameters"]
                              .as<std::vector<MaterialPrimitiveParameter>>();
    textureParameters =
        root["TextureParameters"].as<std::vector<MaterialTextureParameter>>();
    setMaterial(as<Material>(assetManager.get(matHandle)));
    assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
}
