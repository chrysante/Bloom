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
                            std::invocable<ParamType const&> auto setter) {
    using ValueType = std::remove_cvref_t<decltype(existingParams[0].value)>;
    using Map =
        utl::hashmap<std::pair<MaterialDataType, std::string>, ValueType>;
    auto backup = existingParams | transform([](auto& param) {
        return std::pair{ std::pair{ param.type, param.name }, param.value };
    }) | ranges::to<Map>;
    existingParams = newParams;
    for (auto& param: existingParams) {
        auto itr = backup.find({ param.type, param.name });
        if (itr != backup.end()) {
            param.value = itr->second;
        }
        std::invoke(setter, param);
    }
}

void MaterialInstance::setMaterial(Reference<Material> material) {
    if (material) {
        cpuBuffer.resize(material->paramBufferSize);
        amendParameters(primitiveParameters, material->primitiveParameters,
                        [&](MaterialPrimitiveParameter const& param) {
            setParameter(param.value.data(), param.bufferOffset,
                         layoutSize(param.type));
        });
        amendParameters(textureParameters, material->textureParameters,
                        [&](MaterialTextureParameter const& param) {
            using enum AssetRepresentation;
            setTexture(param.vertexBindingPoint, param.fragmentBindingPoint,
                       getAsset<Texture2D>(param.value, GPU));
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
