#include "Bloom/Graphics/Material/MaterialInstance.h"

#include <cstring>

#include "Bloom/Asset/AssetManager.h"
#include "Bloom/Asset/Serialization.h"
#include "Bloom/GPU/HardwareDevice.h"

namespace bloom {

void MaterialInstance::setMaterial(Reference<Material> material) {
    cpuBuffer.resize(material->paramBufferSize);
    if (material) {
        primitiveParameters = material->primitiveParameters;
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
    return root;
}

void MaterialInstance::deserialize(YAML::Node const& root,
                                   AssetManager& assetManager) {
    auto const matHandle = root["Material"].as<AssetHandle>();
    setMaterial(as<Material>(assetManager.get(matHandle)));
    assetManager.makeAvailable(matHandle, AssetRepresentation::GPU);
}

} // namespace bloom
