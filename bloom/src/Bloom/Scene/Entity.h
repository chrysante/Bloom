#ifndef BLOOM_SCENE_ENTITY_H
#define BLOOM_SCENE_ENTITY_H

#include <iosfwd>

#include <entt/entt.hpp>
#include <utl/common.hpp>
#include <utl/hash.hpp>
#include <utl/utility.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Scene/Components/ComponentBase.h"

namespace bloom {

struct BLOOM_API EntityID {
public:
    friend class Scene;
    using RawType = std::underlying_type_t<entt::entity>;

public:
    EntityID() = default;
    EntityID(entt::entity value): _value(value) {}
    explicit EntityID(RawType value): _value((entt::entity)value) {}

    entt::entity value() const { return _value; }
    RawType raw() const { return utl::to_underlying(_value); }

    explicit operator bool() const { return value() != entt::null; }

    friend bool operator==(EntityID const&, EntityID const&) = default;

private:
    entt::entity _value = entt::null;
};

BLOOM_API std::ostream& operator<<(std::ostream&, EntityID);

class Scene;

template <bool IsConst>
class BLOOM_API EntityHandleEx: public EntityID {
    using SceneT = std::conditional_t<IsConst, Scene const, Scene>;
    template <bool>
    friend class EntityHandleEx;

public:
    EntityHandleEx() = default;
    EntityHandleEx(EntityID id, SceneT* scene): EntityID(id), _scene(scene) {}
    EntityHandleEx(EntityHandleEx<false> rhs)
        requires(IsConst)
        : EntityID(rhs), _scene(rhs._scene) {}

    SceneT& scene() const { return *_scene; }

    EntityID ID() const { return *this; }

    using EntityID::operator=;

    template <ComponentType T>
    bool has() const {
        return tScene<T>()->template hasComponent<T>(*this);
    }

    template <ComponentType T, ComponentType... U>
    bool hasAll() const {
        return (has<T>() && ... && has<U>());
    }

    template <ComponentType T>
    T& get() const
        requires(!IsConst)
    {
        return tScene<T>()->template getComponent<T>(*this);
    }

    template <ComponentType T>
    T const& get() const {
        return tScene<T>()->template getComponent<T>(*this);
    }

    template <ComponentType T, ComponentType... U>
    void add(T&& first, U&&... rest) const
        requires(!IsConst)
    {
        tScene<T>()->template addComponent(*this, std::forward<T>(first));
        (..., add(std::forward<U>(rest)));
    }

    template <ComponentType T>
    void remove() const
        requires(!IsConst)
    {
        tScene<T>()->template removeComponent<T>(*this);
    }

private:
    template <typename>
    auto* tScene() const {
        return _scene;
    }

private:
    SceneT* _scene = nullptr;
};

using EntityHandle = EntityHandleEx<false>;
using ConstEntityHandle = EntityHandleEx<true>;

} // namespace bloom

template <>
struct std::hash<bloom::EntityID> {
    std::size_t operator()(bloom::EntityID id) const {
        return std::hash<decltype(id.raw())>{}(id.raw());
    }
};

template <bool IsConst>
struct std::hash<bloom::EntityHandleEx<IsConst>> {
    std::size_t operator()(bloom::EntityHandleEx<IsConst> const& entity) const {
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
