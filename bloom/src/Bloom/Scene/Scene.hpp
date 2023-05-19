#pragma once

#include <string>

#include <entt/entt.hpp>
#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"
#include "Bloom/Scene/Components/AllComponents.hpp"
#include "Bloom/Scene/Entity.hpp"

namespace bloom {

class BLOOM_API Scene: public Asset {
public:
    using Asset::Asset;

    EntityHandle createEntity(std::string_view name);

    EntityHandle createEmptyEntity();

    EntityHandle createEmptyEntity(EntityID hint);

    EntityHandle getHandle(EntityID id) { return EntityHandle(id, this); }

    ConstEntityHandle getHandle(EntityID id) const {
        return ConstEntityHandle(id, this);
    }

    EntityHandle cloneEntity(EntityID);

    void deleteEntity(EntityID);

    template <ComponentType T>
    bool hasComponent(EntityID entity) const {
        return _registry.any_of<T>(entity.value());
    }

    template <ComponentType T>
    T& getComponent(EntityID entity) {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        return _registry.get<T>(entity.value());
    }

    template <ComponentType T>
    T const& getComponent(EntityID entity) const {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        return _registry.get<T>(entity.value());
    }

    template <ComponentType T>
    void addComponent(EntityID entity, T&& component) {
        assert(!hasComponent<std::decay_t<T>>(entity) &&
               "ComponentType already present");
        _registry.emplace<std::decay_t<T>>(entity.value(),
                                           UTL_FORWARD(component));
    }

    template <ComponentType T>
    void removeComponent(EntityID entity) {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        _registry.remove<T>(entity.value());
    }

    void clear() { _registry.clear(); }

    bool empty() const { return _registry.empty(); }

    template <typename... Components>
    auto view() {
        return _registry.view<Components...>();
    }
    template <typename... Components>
    auto view() const {
        return _registry.view<Components...>();
    }

    auto each(std::invocable<entt::entity> auto&& f) {
        _registry.each(UTL_FORWARD(f));
    }
    auto each(std::invocable<entt::entity> auto&& f) const {
        _registry.each(UTL_FORWARD(f));
    }

    Scene copy();

    /// MARK: Serialize
    YAML::Node serialize() const;

    void deserialize(YAML::Node const&, AssetManager&);

    /// MARK: Hierarchy functionality. Maybe extract this later
    void parent(EntityID child, EntityID parent);

    void unparent(EntityID);

    // returns true if 'descendend' and 'ancestor' are the same
    bool descendsFrom(EntityID descendend, EntityID ancestor) const;

    utl::small_vector<EntityID> gatherRoots() const;

    utl::small_vector<EntityID> gatherChildren(EntityID parent) const;

    bool isLeaf(EntityID) const;

    mtl::float4x4 calculateTransformRelativeToWorld(EntityID) const;

private:
    entt::registry _registry;
};

} // namespace bloom
