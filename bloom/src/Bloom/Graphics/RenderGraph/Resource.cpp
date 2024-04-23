#include "Bloom/Graphics/RenderGraph/Resource.h"

#include <algorithm>
#include <range/v3/algorithm.hpp>

#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/Graphics/Material/MaterialInstance.h"

using namespace bloom;
using namespace rg;
using namespace vml;

void Resource::addUser(PassInterfaceBase* user) {
    if (!ranges::contains(_users, user)) {
        _users.push_back(user);
    }
}

void Resource::assignUsers() {
    visit(*this, [this]<typename ResourceType>(ResourceType& self) {
        for (auto* user: users()) {
            if (auto* handle = user->handle<ResourceType::Type>()) {
                *handle = self.handle();
            }
        }
    });
}

void BufferResource::assign(BufferHandle buffer) {
    _handle = std::move(buffer);
    assignUsers();
}

void TextureResource::assign(TextureHandle texture) {
    _handle = std::move(texture);
    assignUsers();
}

void GeometryResource::add(Reference<StaticMeshRenderer> mesh,
                           Reference<MaterialInstance> material,
                           vml::float4x4 const& transform) {
    objects.emplace_back(std::move(mesh), std::move(material),
                         vml::transpose(transform));
}

void GeometryResource::clear() { objects.clear(); }

static constexpr auto RenderGeometryOrder = [](Geometry::reference a,
                                               Geometry::reference b) {
    auto& aMatInst = *a.materialInstance;
    auto* const aMat = aMatInst.material();
    auto& bMatInst = *b.materialInstance;
    auto* const bMat = bMatInst.material();

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
    if (transforms.size() < transformBufferSize) {
        transforms =
            device.createBuffer({ .size = transformBufferSize,
                                  .storageMode = StorageMode::Managed });
    }
    device.fillManagedBuffer(transforms, objects.data().transform,
                             transformBufferSize);
}

void LightsResource::add(PointLight const& light) {
    pointLights.push_back(light);
}

void LightsResource::add(SpotLight const& light) {
    spotLights.push_back(light);
}

void LightsResource::add(DirectionalLight const& light) {
    dirLights.push_back(light);
}

void LightsResource::add(SkyLight const& light) { skyLights.push_back(light); }

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
