#include "Bloom/Graphics/Material/MaterialInstance.h"

#include <cstring>

#include <range/v3/view.hpp>
#include <utl/hashtable.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Texture.h"

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

static void amendPrimitives(
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

static void amendTextures(
    AssetManager& AM, MaterialInstance& inst,
    std::vector<MaterialTextureParameter>& existingParams,
    std::vector<MaterialTextureParameter> const& newParams) {
    auto backup =
        existingParams | transform([](auto& param) {
        return std::pair{ std::pair{ param.type, param.name },
                          param.defaultValue };
    }) |
        ranges::to<utl::hashmap<std::pair<MaterialDataType, std::string>,
                                AssetHandle>>;
    existingParams = newParams;
    for (auto& param: existingParams) {
        auto itr = backup.find({ param.type, param.name });
        if (itr != backup.end()) {
            param.defaultValue = itr->second;
        }
        inst.setTexture(param.vertexBindingPoint, param.fragmentBindingPoint,
                        as<Texture2D>(AM.get(param.defaultValue,
                                             AssetRepresentation::GPU)));
    }
}

void MaterialInstance::setMaterial(Reference<Material> material) {
    if (material) {
        cpuBuffer.resize(material->paramBufferSize);
        amendPrimitives(*this, primitiveParameters,
                        material->primitiveParameters);
        AssetManager* AM = nullptr;
        Application::DispatchMessage(DispatchToken::Now,
                                     GetAssetManager{ &AM });
        amendTextures(*AM, *this, textureParameters,
                      material->textureParameters);
        material->instances.insert(weak_from_this());
    }
    mMaterial = std::move(material);
}

void MaterialInstance::setParameter(void* data, size_t offset, size_t size) {
    BL_ASSERT(cpuBuffer.size() >= offset + size);
    std::memcpy(cpuBuffer.data() + offset, data, size);
    mDirty = true;
}

void MaterialInstance::setTexture(int vertBinding, int fragBinding,
                                  Reference<Texture2D> texture) {
    textures[{ vertBinding, fragBinding }] = std::move(texture);
}

Reference<Texture2D> MaterialInstance::getTexture(int vertBinding,
                                                  int fragBinding) const {
    auto itr = textures.find({ vertBinding, fragBinding });
    return itr != textures.end() ? itr->second : nullptr;
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

void MaterialInstance::deserialize(YAML::Node const& root, AssetManager& AM) {
    auto matHandle = root["Material"].as<AssetHandle>();
    primitiveParameters = root["PrimitiveParameters"]
                              .as<std::vector<MaterialPrimitiveParameter>>();
    textureParameters =
        root["TextureParameters"].as<std::vector<MaterialTextureParameter>>();
    setMaterial(as<Material>(AM.get(matHandle, AssetRepresentation::GPU)));
}
