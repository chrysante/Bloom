#ifndef BLOOM_SCENE_ENTITY_H
#define BLOOM_SCENE_ENTITY_H

#include <entt/entt.hpp>
#include <utl/common.hpp>
#include <utl/hash.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"
#include "Bloom/Scene/Components/Hierarchy.h"
#include "Bloom/Scene/Components/Tag.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Components/Visibility.h"
#include "Bloom/Scene/EntityID.h"

namespace bloom {

class Scene;

/// A wrapper around an entity ID that also stores a pointer to the enclosing
/// scene and provides a more object oriented entity interface. The template
/// parameter \p SceneT must be either `Scene` or `Scene const`.
template <typename SceneT>
class BLOOM_API EntityHandleEx: public EntityID {
    template <typename>
    friend class EntityHandleEx;

    static constexpr bool IsConst = std::is_const_v<SceneT>;

    template <typename T>
    using CopyConst = std::conditional_t<IsConst, T const, T>;

public:
    /// Construct an empty handle
    EntityHandleEx() = default;

    ///
    EntityHandleEx(EntityID ID, SceneT* scene): EntityID(ID), _scene(scene) {}

    /// Implicit `EntityHandle -> ConstEntityHandle` conversion
    EntityHandleEx(EntityHandleEx<std::remove_const_t<SceneT>> rhs)
        requires(IsConst)
        : EntityID(rhs), _scene(rhs._scene) {}

    /// \Returns the associated scene
    SceneT& scene() const { return *_scene; }

    /// \Returns the underlying entity ID
    EntityID ID() const { return *this; }

    /// \Returns `true` if this entity has a component of type \p T
    template <ComponentType T>
    bool has() const {
        return scene().template hasComponent<T>(ID());
    }

    /// \Return `true` if this entity has all of the given component types
    template <ComponentType T, ComponentType... U>
    bool hasAll() const {
        return (has<T>() && ... && has<U>());
    }

    /// \Return `true` if this entity has any of the given component types
    template <ComponentType T, ComponentType... U>
    bool hasAny() const {
        return (has<T>() || ... || has<U>());
    }

    /// \Returns the component \p T of this entity
    /// \pre Component must be present
    template <DataComponent T>
    T& get() const
        requires(!IsConst)
    {
        return scene().template getComponent<T>(ID());
    }

    /// \overload
    template <DataComponent T>
    T const& get() const {
        return scene().template getComponent<T>(ID());
    }

    /// \Returns the component \p T of this entity if present or null
    template <DataComponent T>
    T* tryGet() const
        requires(!IsConst)
    {
        return scene().template tryGetComponent<T>(ID());
    }

    /// \overload
    template <DataComponent T>
    T const* tryGet() const {
        return scene().template tryGetComponent<T>(ID());
    }

    /// Adds the component \p component to this entity
    /// \pre The component must not be present
    template <DataComponent T>
    std::remove_cvref_t<T>& add(T&& component) const
        requires(!IsConst)
    {
        return scene().addComponent(ID(), std::forward<T>(component));
    }

    template <ComponentType T, typename... Args>
    decltype(auto) add(Args&&... args) const
        requires(!IsConst)
    {
        return scene().template addComponent<T>(ID(),
                                                std::forward<Args>(args)...);
    }

    /// Adds the components \p components to this entity
    /// \pre The components must not be present
    template <DataComponent... T>
    void add(T&&... components) const
        requires(!IsConst)
    {
        scene().addComponents(ID(), std::forward<T>(components)...);
    }

    ///
    template <ComponentType T, typename... Args>
    void tryAdd(Args&&... args) const
        requires(!IsConst)
    {
        auto& s = scene();
        if (!s.template hasComponent<T>(ID())) {
            s.template addComponent<T>(ID(), std::forward<Args>(args)...);
        }
    }

    /// Adds the components \p components to this entity if not already present
    template <DataComponent... T>
    void tryAdd(T&&... components) const
        requires(!IsConst)
    {
        auto& s = scene();
        ([&] {
            using DT = std::decay_t<T>;
            if (!s.template hasComponent<DT>(ID())) {
                s.addComponents(ID(), std::forward<T>(components));
            }
        }(), ...);
    }

    /// Removes the component types \p T... from if present
    template <ComponentType... T>
    void remove() const
        requires(!IsConst)
    {
        scene().template removeComponents<T...>(ID());
    }

    /// \Returns `true` if this handle refers to a valid entity
    explicit operator bool() const { return bool(ID()); }

    ///
    EntityHandleEx clone() const
        requires(!IsConst)
    {
        return scene().cloneEntity(*this);
    }

    /// \Returns `true` if this entity has no visibility component or if the
    /// visibility component is set to visible
    bool isVisible() const {
        auto* vis = tryGet<VisibilityComponent>();
        if (vis && !vis->value) {
            return false;
        }
        if (auto parent = getParent()) {
            return parent.isVisible();
        }
        return true;
    }

    /// \Returns the name of this entity
    std::string_view name() const {
        if (auto* t = tryGet<TagComponent>()) {
            return t->name;
        }
        return {};
    }

    /// See Scene.h
    vml::float4x4 calculateWorldTransformMatrix() const {
        return scene().calculateWorldTransformMatrix(*this);
    }

    /// See Scene.h
    Transform calculateWorldTransform() const {
        return scene().calculateWorldTransform(*this);
    }

    /// \Returns the parent entity if this entity has a hierarchy component
    EntityHandleEx getParent() const {
        return getHierarchyImpl(&HierarchyComponent::parent);
    }

    /// \Returns the next sibling entity if this entity has a hierarchy
    /// component
    EntityHandleEx getNextSibling() const {
        return getHierarchyImpl(&HierarchyComponent::nextSibling);
    }

    /// \Returns the previous sibling entity if this entity has a hierarchy
    /// component
    EntityHandleEx getPrevSibling() const {
        return getHierarchyImpl(&HierarchyComponent::prevSibling);
    }

    /// \Returns the first child entity if this entity has a hierarchy component
    EntityHandleEx getFirstChild() const {
        return getHierarchyImpl(&HierarchyComponent::firstChild);
    }

    /// \Returns the last child entity if this entity has a hierarchy component
    EntityHandleEx getLastChild() const {
        return getHierarchyImpl(&HierarchyComponent::lastChild);
    }

    ///
    void detachFromHieararchy() const
        requires(!IsConst)
    {
        scene().detachFromHierarchy(ID());
    }

    /// \Returns a description of all components in the entity
    std::string debugDescription() const;

private:
    EntityHandleEx getHierarchyImpl(auto f) const {
        auto* hierarchy = tryGet<HierarchyComponent>();
        if (!hierarchy) {
            return {};
        }
        return scene().getHandle(std::invoke(f, hierarchy));
    }

    SceneT* _scene = nullptr;
};

/// Typedef for handle of mutable entity
using EntityHandle = EntityHandleEx<Scene>;

/// Typedef for handle of readonly entity
using ConstEntityHandle = EntityHandleEx<Scene const>;

BLOOM_API std::ostream& operator<<(std::ostream& ostream, EntityHandle entity);
BLOOM_API std::ostream& operator<<(std::ostream& ostream,
                                   ConstEntityHandle entity);

} // namespace bloom

template <typename SceneT>
struct std::hash<bloom::EntityHandleEx<SceneT>> {
    std::size_t operator()(bloom::EntityHandleEx<SceneT> const& entity) const {
        return utl::hash_combine(entity.ID(), &entity.scene());
    }
};

#endif // BLOOM_SCENE_ENTITY_H
