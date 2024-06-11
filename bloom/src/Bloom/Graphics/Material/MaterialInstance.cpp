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

template <typename ParamType>
static void amendParameters(std::vector<ParamType>& existingParams,
                            std::vector<ParamType> const& newParams,
                            auto setter) {
    using ValueType = std::remove_cvref_t<decltype(existingParams[0].value)>;
    using Map =
        utl::hashmap<std::pair<MaterialDataType, std::string>, ValueType>;
    auto backup = existingParams | transform([](auto& param) {
        return std::pair{ std::pair{ param.type, param.name }, param.value };
    }) | ranges::to<Map>;
    existingParams = newParams;
    for (auto [index, param]: existingParams | enumerate) {
        auto itr = backup.find({ param.type, param.name });
        if (itr != backup.end()) {
            param.value = itr->second;
        }
        std::invoke(setter, param, index);
    }
}

void MaterialInstance::setMaterial(Reference<Material> material) {
    if (material) {
        cpuBuffer.resize(material->paramBufferSize);
        amendParameters(primitiveParameters, material->primitiveParameters,
                        [&](MaterialPrimitiveParameter const& param, size_t) {
            setParameter(param.value.data(), param.bufferOffset,
                         layoutSize(param.type));
        });
        textures.resize(material->textureParameters.size());
        amendParameters(textureParameters, material->textureParameters,
                        [&](MaterialTextureParameter const& param,
                            size_t index) {
            using enum AssetRepresentation;
            setTexture(index, getAsset<Texture2D>(param.value, GPU));
        });
        material->instances.insert(weak_from_this());
    }
    mMaterial = std::move(material);
}

void MaterialInstance::setParameter(void const* data, size_t offset,
                                    size_t size) {
    BL_ASSERT(cpuBuffer.size() >= offset + size);
    std::memcpy(cpuBuffer.data() + offset, data, size);
    mDirty = true;
}

void MaterialInstance::setTexture(size_t index, Reference<Texture2D> texture) {
    textures[index] = std::move(texture);
}

Reference<Texture2D> MaterialInstance::getTexture(size_t index) const {
    return textures[index];
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
