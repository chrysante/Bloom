#include "Bloom/Scene/Entity.hpp"

#include <ostream>

#include <utl/utility.hpp>

namespace bloom {

BLOOM_API std::ostream& operator<<(std::ostream& str, EntityID id) {
    return str << utl::to_underlying(id.value());
}

} // namespace bloom
