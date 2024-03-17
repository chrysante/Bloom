#ifndef BLOOM_SCENE_ENTITY_H
#define BLOOM_SCENE_ENTITY_H

#include <iosfwd>

#include <entt/entt.hpp>
#include <utl/common.hpp>
#include <utl/hash.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

///
struct BLOOM_API EntityID {
public:
    friend class Scene;
    using RawType = std::underlying_type_t<entt::entity>;

public:
    ///
    EntityID() = default;

    ///
    EntityID(entt::entity value): _value(value) {}

    ///
    explicit EntityID(RawType value): _value((entt::entity)value) {}

    ///
    entt::entity value() const { return _value; }

    ///
    RawType raw() const { return utl::to_underlying(_value); }

    ///
    explicit operator bool() const { return value() != entt::null; }

    ///
    friend bool operator==(EntityID const&, EntityID const&) = default;

private:
    entt::entity _value = entt::null;
};

///
BLOOM_API std::ostream& operator<<(std::ostream&, EntityID);

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
    EntityHandleEx(EntityID id, SceneT* scene): EntityID(id), _scene(scene) {}

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
    template <ComponentType T>
    T& get() const
        requires(!IsConst)
    {
        return scene().template getComponent<T>(*this);
    }

    /// \overload
    template <ComponentType T>
    T const& get() const {
        return scene().template getComponent<T>(*this);
    }

    /// Adds the components \p components to this entity
    /// \pre Components must not be present
    template <ComponentType... T>
    void add(T&&... components) const
        requires(!IsConst)
    {
        scene().template addComponents(*this, std::forward<T>(components)...);
    }

    /// Adds the components \p components to this entity if not already present
    template <ComponentType... T>
    void tryAdd(T&&... components) const
        requires(!IsConst)
    {
        auto& s = scene();
        (
            [&] {
            using DT = std::decay_t<T>;
            if (!s.template hasComponent<DT>(*this)) {
                s.template addComponents(*this, std::forward<T>(components));
            }
        }(),
            ...);
    }

    /// Removes the component types \p T... from if present
    template <ComponentType... T>
    void remove() const
        requires(!IsConst)
    {
        scene().template removeComponents<T...>(*this);
    }

    /// \Returns `true` if this handle refers to a valid entity
    explicit operator bool() const { return bool(ID()); }

private:
    SceneT* _scene = nullptr;
};

/// Typedef for handle of mutable entity
using EntityHandle = EntityHandleEx<Scene>;

/// Typedef for handle of readonly entity
using ConstEntityHandle = EntityHandleEx<Scene const>;

} // namespace bloom

template <>
struct std::hash<bloom::EntityID> {
    std::size_t operator()(bloom::EntityID id) const {
        return std::hash<decltype(id.raw())>{}(id.raw());
    }
};

template <typename SceneT>
struct std::hash<bloom::EntityHandleEx<SceneT>> {
    std::size_t operator()(bloom::EntityHandleEx<SceneT> const& entity) const {
        return utl::hash_combine(entity.ID(), &entity.scene());
    }
};

template <>
struct YAML::convert<bloom::EntityID> {
    static Node encode(bloom::EntityID const& e) {
        Node node;
        node = static_cast<std::int32_t>(e.raw());
        return node;
    }

    static bool decode(Node const& node, bloom::EntityID& e) {
        e = bloom::EntityID((bloom::EntityID::RawType)node.as<std::int32_t>());
        return true;
    }
};

#endif // BLOOM_SCENE_ENTITY_H
