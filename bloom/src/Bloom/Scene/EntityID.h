#ifndef EntityID_h
#define EntityID_h

#include <iosfwd>
#include <type_traits>

#include <entt/entt.hpp>

#include "Bloom/Core/Base.h"

namespace bloom {

/// Unique entity identifier
struct EntityID {
public:
    friend class Scene;
    using RawType = std::underlying_type_t<entt::entity>;

    ///
    EntityID() = default;

    /// Construct from an `entt::entity`
    EntityID(entt::entity value): _value(value) {}

    /// Construct from a raw ID value
    explicit EntityID(RawType value): _value((entt::entity)value) {}

    /// \Returns the underlying `entt::entity`
    entt::entity value() const { return _value; }

    /// \Returns the underlying identifier value
    RawType raw() const { return utl::to_underlying(_value); }

    /// \Returns `true` is this is not the null entity
    explicit operator bool() const { return value() != entt::null; }

    /// Compares the underlying values
    friend bool operator==(EntityID const&, EntityID const&) = default;

private:
    entt::entity _value = entt::null;
};

///
BLOOM_API std::ostream& operator<<(std::ostream& ostream, EntityID ID);

} // namespace bloom

template <>
struct std::hash<bloom::EntityID> {
    std::size_t operator()(bloom::EntityID ID) const {
        return std::hash<decltype(ID.raw())>{}(ID.raw());
    }
};

template <>
struct YAML::convert<bloom::EntityID> {
    static Node encode(bloom::EntityID const& e) {
        Node node;
        node = static_cast<int32_t>(e.raw());
        return node;
    }

    static bool decode(Node const& node, bloom::EntityID& e) {
        e = bloom::EntityID((bloom::EntityID::RawType)node.as<int32_t>());
        return true;
    }
};

#endif /* EntityID_h */
