#include "Bloom/Scene/Entity.h"

#include <ostream>

#include <utl/utility.hpp>

#include "Bloom/Scene/Scene.h"

using namespace bloom;

std::ostream& bloom::operator<<(std::ostream& str, EntityID id) {
    return str << utl::to_underlying(id.value());
}

std::ostream& bloom::operator<<(std::ostream& str, EntityHandle entity) {
    return str << entity.name() << "[" << entity.ID() << "]";
}

std::ostream& bloom::operator<<(std::ostream& str, ConstEntityHandle entity) {
    return str << entity.name() << "[" << entity.ID() << "]";
}
