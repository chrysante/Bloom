#include "Bloom/Graphics/RenderGraph/Resource.h"

#include <algorithm>
#include <range/v3/algorithm.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"
#include "Bloom/Shader/SharedTypes.h"

using namespace bloom;
using namespace vml;

void Resource::addUser(PassTerminal* user) {
    if (!ranges::contains(_users, user)) {
        _users.push_back(user);
    }
}

void Resource::reallocate(HardwareDevice& device,
                          FrameParameters const& params) {
    visit(*this, [&](auto& r) { r.doReallocate(device, params); });
    assignUsers();
}

void Resource::assignUsers() {
    visit(*this, [this]<typename ResourceType>(ResourceType& self) {
        for (auto* user: users()) {
            if (auto* handle = user->handle()) {
                *(typename ResourceType::ViewType*)handle = self.handle();
            }
        }
    });
}

void BufferResource::assign(BufferHandle buffer) {
    _handle = std::move(buffer);
    assignUsers();
}

void BufferResource::doReallocate(HardwareDevice& device,
                                  FrameParameters const& params) {
    auto newDesc = idf(params);
    if (newDesc == desc) {
        return;
    }
    desc = newDesc;
    _handle = device.createBuffer({
        .size = desc.size,
        .storageMode = StorageMode::Managed // For now!
    });
}

void TextureResource::assign(TextureHandle texture) {
    _handle = std::move(texture);
    assignUsers();
}

static bool validate(TextureInterfaceDescriptor const& desc) {
    if (desc.type == TextureType::Texture2DArray) {
        if (desc.arrayLength < 1) {
            return false;
        }
    }
    return true;
}

void TextureResource::doReallocate(HardwareDevice& device,
                                   FrameParameters const& params) {
    auto newDesc = idf(params);
    if (newDesc == desc) {
        return;
    }
    desc = newDesc;
    if (!validate(desc)) {
        _handle = {};
        return;
    }
    _handle = device.createTexture({ .type = desc.type,
                                     .pixelFormat = desc.pixelFormat,
                                     .size = desc.size,
                                     .mipmapLevelCount = desc.mipmapLevelCount,
                                     .sampleCount = desc.sampleCount,
                                     .arrayLength = desc.arrayLength,
                                     .storageMode = desc.storageMode,
                                     .usage = desc.usage });
}

void GeometryResource::add(Reference<StaticMeshRenderer> mesh,
                           Reference<MaterialInstance> material,
                           vml::float4x4 const& transform, uint32_t entityID) {
    objects.emplace_back(std::move(mesh), std::move(material),
                         vml::transpose(transform), entityID);
}

void GeometryResource::clear() { objects.clear(); }

static constexpr auto RenderGeometryOrder = [](Geometry::const_reference a,
                                               Geometry::const_reference b) {
    auto& aMatInst = *a.materialInstance;
    auto* aMat = aMatInst.material();
    auto& bMatInst = *b.materialInstance;
    auto* bMat = bMatInst.material();
    if (aMat == bMat) { // Sort by material first
        if (a.materialInstance == b.materialInstance) { // Then by instance
            return a.mesh < b.mesh;                     // Then by mesh
        }
        return a.materialInstance < b.materialInstance;
    }
    return aMat < bMat;
};

void GeometryResource::prepare(HardwareDevice& device) {
    std::sort(objects.begin(), objects.end(), RenderGeometryOrder);
    size_t transformBufferSize = objects.size() * sizeof(float4x4);
    if (transformBufferSize == 0) {
        return;
    }
    size_t idBufferSize = objects.size() * sizeof(uint32_t);
    if (transforms.size() < transformBufferSize) {
        transforms =
            device.createBuffer({ .size = transformBufferSize,
                                  .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(transforms, objects.data().transform,
                             transformBufferSize);
    if (IDs.size() < idBufferSize) {
        IDs = device.createBuffer(
            { .size = idBufferSize, .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(IDs, objects.data().entityID, idBufferSize);
}

void LightsResource::add(PointLightRenderData const& light) {
    pointLights.push_back(light);
}

void LightsResource::add(SpotLightRenderData const& light) {
    spotLights.push_back(light);
}

void LightsResource::add(DirectionalLightRenderData const& light) {
    dirLights.push_back(light);
}

void LightsResource::add(SkyLightRenderData const& light) {
    skyLights.push_back(light);
}

void LightsResource::clear() {
    pointLights.clear();
    spotLights.clear();
    dirLights.clear();
    skyLights.clear();
}

template <typename L>
static uint32_t copyLights(std::type_identity_t<std::span<L const>> source,
                           L* dest) {
    size_t const MaxNumLights = 32;
    if (source.size() > MaxNumLights) {
        Logger::Warn("Max number of lights exceeded");
        source = source.subspan(0, MaxNumLights);
    }
    ranges::copy(source, dest);
    return utl::narrow_cast<uint32_t>(source.size());
}

void LightsResource::prepare(HardwareDevice& device) {
    LightBufferContents lights;
    lights.numPointLights = copyLights(pointLights, lights.pointLights);
    lights.numSpotLights = copyLights(spotLights, lights.spotLights);
    lights.numDirLights = copyLights(dirLights, lights.dirLights);
    lights.numSkyLights = copyLights(skyLights, lights.skyLights);
    if (!buffer) {
        buffer = device.createBuffer(
            { .size = sizeof lights, .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(buffer, &lights, sizeof lights);
}
