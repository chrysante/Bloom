#include "Bloom/Graphics/RenderGraph/ResourceNodes.h"

using namespace bloom;
using namespace rg;

void Resource::addUser(PassInterfaceBase* user) {
    if (!ranges::contains(_users, user)) {
        _users.push_back(user);
    }
}

void Resource::assignUsers() {
    visit(*this, [this]<typename R>(R& self) {
        for (auto* user: users()) {
            using I = typename ToInterfaceType<R>::type;
            I& interface = user->get<I>();
            *interface.handle = self.handle();
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
    objects.emplace_back(std::move(mesh), std::move(material), transform);
}

void GeometryResource::clear() { objects.clear(); }

void GeometryResource::prepare() { BL_UNIMPLEMENTED(); }

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

void LightsResource::prepare() { BL_UNIMPLEMENTED(); }
