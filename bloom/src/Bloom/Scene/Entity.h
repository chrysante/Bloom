#ifndef BLOOM_SCENE_ENTITY_H
#define BLOOM_SCENE_ENTITY_H

#include <entt/entt.hpp>
#include <utl/common.hpp>
#include <utl/hash.hpp>
#include <utl/utility.hpp>
#include <utl/vector.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Core/Yaml.h"
#include "Bloom/Scene/Components/ComponentBase.h"
#include "Bloom/Scene/Components/Hierarchy.h"
#include "Bloom/Scene/Components/Tag.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Components/Visibility.h"
#include "Bloom/Scene/EntityID.h"

namespace bloom {

class Scene;
template <typename ScenePtr, bool IsConst>
class BLOOM_API EntityHandleEx;

/// Typedef for a handle of a mutable entity
using EntityHandle = EntityHandleEx<Scene*, false>;

/// Typedef for a handle of a readonly entity
using ConstEntityHandle = EntityHandleEx<Scene const*, true>;

/// Typedef for a handle of a mutable entity with a strong reference to the
/// scene
using StrongEntityHandle = EntityHandleEx<Reference<Scene>, false>;

/// A wrapper around an entity ID that also stores a pointer to the enclosing
/// scene and provides a more object oriented entity interface. The template
/// parameter \p ScenePtr must be either `Scene [const]*` or `Reference<Scene
/// [const]>`.
template <typename ScenePtr, bool IsConst>
class BLOOM_API EntityHandleEx: public EntityID {
    template <typename, bool>
    friend class EntityHandleEx;

    template <typename T>
    using CopyConst = std::conditional_t<IsConst, T const, T>;

    static ScenePtr makePointer(auto* p) {
        if constexpr (std::is_pointer_v<ScenePtr>) {
            return p;
        }
        else {
            return p->shared_from_this();
        }
    }

public:
    /// Construct an empty handle
    EntityHandleEx() = default;

    ///
    EntityHandleEx(EntityID ID, ScenePtr scene): EntityID(ID), _scene(scene) {}

    /// Implicit `EntityHandle -> ConstEntityHandle` conversion
    EntityHandleEx(EntityHandle rhs)
        requires IsConst && std::is_pointer_v<ScenePtr>
        : EntityID(rhs.ID()), _scene(rhs.scene()) {}

    /// Implicit `StrongEntityHandle -> [Const]EntityHandle` conversion
    template <bool ConstRhs>
    EntityHandleEx(EntityHandleEx<Reference<Scene>, ConstRhs> rhs)
        requires std::is_pointer_v<ScenePtr> && (IsConst || (!ConstRhs))
        : EntityID(rhs.ID()), _scene(rhs.scene().get()) {}

    /// \Returns the associated scene
    ScenePtr const& scene() const { return _scene; }

    /// \Returns the underlying entity ID
    EntityID ID() const { return *this; }

    /// Creates a strong entity handle from this handle
    StrongEntityHandle strongHandle() const
        requires(!IsConst)
    {
        return StrongEntityHandle(ID(), scene()->shared_from_this());
    }

    /// \Returns `true` if this entity is still valid
    bool isValid() const { return scene()->isValid(ID()); }

    /// \Returns `true` if this entity has a component of type \p T
    template <ComponentType T>
    bool has() const {
        return scene()->template hasComponent<T>(ID());
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
        return scene()->template getComponent<T>(ID());
    }

    /// \overload
    template <DataComponent T>
    T const& get() const {
        return scene()->template getComponent<T>(ID());
    }

    /// \Returns the component \p T of this entity if present or null
    template <DataComponent T>
    T* tryGet() const
        requires(!IsConst)
    {
        return scene()->template tryGetComponent<T>(ID());
    }

    /// \overload
    template <DataComponent T>
    T const* tryGet() const {
        return scene()->template tryGetComponent<T>(ID());
    }

    /// Adds the component \p component to this entity
    /// \pre The component must not be present
    template <DataComponent T>
    std::remove_cvref_t<T>& add(T&& component) const
        requires(!IsConst)
    {
        return scene()->addComponent(ID(), std::forward<T>(component));
    }

    template <ComponentType T, typename... Args>
    decltype(auto) add(Args&&... args) const
        requires(!IsConst)
    {
        return scene()->template addComponent<T>(ID(),
                                                 std::forward<Args>(args)...);
    }

    /// Adds the components \p components to this entity
    /// \pre The components must not be present
    template <DataComponent... T>
    void add(T&&... components) const
        requires(!IsConst)
    {
        scene()->addComponents(ID(), std::forward<T>(components)...);
    }

    ///
    template <ComponentType T, typename... Args>
    bool tryAdd(Args&&... args) const
        requires(!IsConst)
    {
        auto& s = scene();
        if (!s->template hasComponent<T>(ID())) {
            s->template addComponent<T>(ID(), std::forward<Args>(args)...);
            return true;
        }
        return false;
    }

    /// Adds the components \p components to this entity if not already present
    template <DataComponent... T>
    bool tryAdd(T&&... components) const
        requires(!IsConst)
    {
        auto& s = scene();
        return ([&] {
            using DT = std::decay_t<T>;
            if (!s->template hasComponent<DT>(ID())) {
                s->addComponents(ID(), std::forward<T>(components));
                return true;
            }
            return false;
        }() & ...);
    }

    /// \overload for dynamic component type
    void add(ComponentKind type) const
        requires(!IsConst)
    {
        visitComponentType(type, [&]<typename T>(utl::tag<T>) { add<T>(); });
    }

    /// \overload for dynamic component type
    bool tryAdd(ComponentKind type) const
        requires(!IsConst)
    {
        return visitComponentType(type, [&]<typename T>(utl::tag<T>) {
            return tryAdd<T>();
        });
    }

    /// Removes the component types \p T... from this entity
    template <ComponentType... T>
    void remove() const
        requires(!IsConst)
    {
        scene()->template removeComponents<T...>(ID());
    }

    /// Removes the component types \p T... from this entity if present
    template <ComponentType... T>
    bool tryRemove() const
        requires(!IsConst)
    {
        return scene()->template tryRemoveComponents<T...>(ID());
    }

    /// \overload for dynamic component type
    void remove(ComponentKind type) const
        requires(!IsConst)
    {
        visitComponentType(type, [&]<typename T>(utl::tag<T>) { remove<T>(); });
    }

    /// \overload for dynamic component type
    bool tryRemove(ComponentKind type) const
        requires(!IsConst)
    {
        return visitComponentType(type, [&]<typename T>(utl::tag<T>) {
            return tryRemove<T>();
        });
    }

    /// \Returns `true` if this handle refers to a valid entity
    explicit operator bool() const { return bool(ID()); }

    /// Clones this entity and returns a handle to the new entity
    EntityHandleEx clone() const
        requires(!IsConst)
    {
        return EntityHandleEx(scene()->cloneEntity(ID()).ID(), scene());
    }

    /// Deletes this entity from the scene
    void deleteEntity() const
        requires(!IsConst)
    {
        scene()->deleteEntity(ID());
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

    /// See Scene.h
    bool isLeaf() const { return scene()->isLeaf(ID()); }

    /// See Scene.h
    utl::small_vector<EntityID> gatherChildren() const {
        return scene()->gatherChildren(ID());
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
        return scene()->calculateWorldTransformMatrix(ID());
    }

    /// See Scene.h
    Transform calculateWorldTransform() const {
        return scene()->calculateWorldTransform(ID());
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

    /// See Scene.h
    void detachFromHieararchy() const
        requires(!IsConst)
    {
        scene()->detachFromHierarchy(ID());
    }

    /// \Returns a description of all components in the entity
    std::string debugDescription() const;

    ///
    void printDebugDescription() const;

private:
    EntityHandleEx getHierarchyImpl(auto f) const {
        auto* hierarchy = tryGet<HierarchyComponent>();
        if (!hierarchy) {
            return {};
        }
        auto ID = scene()->getHandle(std::invoke(f, hierarchy)).ID();
        return EntityHandleEx(ID, scene());
    }

    ScenePtr _scene{};
};

BLOOM_API std::ostream& operator<<(std::ostream& ostream, EntityHandle entity);
BLOOM_API std::ostream& operator<<(std::ostream& ostream,
                                   ConstEntityHandle entity);

} // namespace bloom

template <typename ScenePtr, bool IsConst>
struct std::hash<bloom::EntityHandleEx<ScenePtr, IsConst>> {
    std::size_t operator()(
        bloom::EntityHandleEx<ScenePtr, IsConst> const& entity) const {
        return utl::hash_combine(entity.ID(), entity.scene());
    }
};

#endif // BLOOM_SCENE_ENTITY_H
