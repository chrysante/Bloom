#ifndef BLOOM_SCENE_SCENE_H
#define BLOOM_SCENE_SCENE_H

#include <string>

#include <entt/entt.hpp>
#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Asset/Asset.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Entity.h"

namespace bloom {

/// Set of entities that can be rendered and interact with each other
class BLOOM_API Scene: public Asset {
public:
    BL_DEFINE_ASSET_CTOR(Scene, Asset)

    /// Creates a new entity with a tag component initialized with \p name
    /// \Returns the handle of the newly created entity
    EntityHandle createEntity(std::string_view name);

    /// Creates a new entity without any components
    /// \Returns the handle of the newly created entity
    EntityHandle createEmptyEntity();

    /// \overload
    EntityHandle createEmptyEntity(EntityID hint);

    /// \Returns a handle for entity \p entity
    EntityHandle getHandle(EntityID entity) {
        return EntityHandle(entity, this);
    }

    /// \overload for `const`
    ConstEntityHandle getHandle(EntityID entity) const {
        return ConstEntityHandle(entity, this);
    }

    /// \Returns a copy of the entity \p entity
    EntityHandle cloneEntity(EntityID entity);

    /// Destroys the entity \p entity
    void deleteEntity(EntityID entity);

    /// \Returns `true` if \p entity has a component of type \p T
    template <ComponentType T>
    bool hasComponent(EntityID entity) const {
        return _registry.any_of<T>(entity.value());
    }

    /// \Returns a reference to the component \p T of entity \p entity
    /// \pre \p entity must have a component of type \p T
    template <ComponentType T>
    T& getComponent(EntityID entity) {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        return _registry.get<T>(entity.value());
    }

    /// \overload for `const`
    template <ComponentType T>
    T const& getComponent(EntityID entity) const {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        return _registry.get<T>(entity.value());
    }

    /// Adds the component \p component to \p entity
    /// \pre \p entity must not have a component of type \p T
    template <ComponentType T>
    void addComponent(EntityID entity, T component) {
        assert(!hasComponent<std::decay_t<T>>(entity) &&
               "ComponentType already present");
        _registry.emplace<std::decay_t<T>>(entity.value(),
                                           std::move(component));
    }

    /// Removes component \p T from \p entity
    /// \pre \p entity must have a component of type \p T
    template <ComponentType T>
    void removeComponent(EntityID entity) {
        assert(hasComponent<T>(entity) && "ComponentType not present");
        _registry.remove<T>(entity.value());
    }

    /// Destroys all entities in this scene
    void clear() { _registry.clear(); }

    /// \Returns a view over all entities which have components of all types \p
    /// Components...
    template <typename... Components>
    auto view() {
        return _registry.view<Components...>();
    }

    /// \overload for `const`
    template <typename... Components>
    auto view() const {
        return _registry.view<Components...>();
    }

    /// Invokes \p callback for every entity in this scene
    template <std::invocable<EntityID> C>
    auto each(C&& callback) {
        _registry.view<entt::entity>().each(
            [&](entt::entity ID) { std::invoke(callback, EntityID(ID)); });
    }

    /// \overload for `const`
    template <std::invocable<EntityID> C>
    auto each(C&& callback) const {
        _registry.view<entt::entity>().each(
            [&](entt::entity ID) { std::invoke(callback, EntityID(ID)); });
    }

    /// \Returns a copy of \p scene
    Reference<Scene> clone();

    /// \Returns a YAML node representing this scene
    YAML::Node serialize() const;

    /// Deserializes the scene stored in \p node
    void deserialize(YAML::Node const& node, AssetManager& AM);

    /// Makes \p parent a parent of \p child
    void parent(EntityID child, EntityID parent);

    /// Extracts \p entity from any hierarchy it is in
    void unparent(EntityID entity);

    /// \Returns `true` if \p descendend is below \p ancestor in the entity
    /// hierarchy or if they are the same entity
    bool descendsFrom(EntityID descendend, EntityID ancestor) const;

    /// \Returns the roots of the entity hierarchy
    utl::small_vector<EntityID> gatherRoots() const;

    /// \Returns all entity directly below \p parent in the entity hierarchy
    utl::small_vector<EntityID> gatherChildren(EntityID parent) const;

    /// \Returns `true` if \p entity has no entities below it in the entity
    /// hierarchy
    bool isLeaf(EntityID entity) const;

    /// Computes the transform matrix of \p entity relative to the world by
    /// traversing the hierarchy and multiplying the transforms of all parents
    /// \pre \p entity must have a transform component
    mtl::float4x4 calculateTransformRelativeToWorld(EntityID entity) const;

private:
    entt::registry _registry;
};

} // namespace bloom

#endif // BLOOM_SCENE_SCENE_H
