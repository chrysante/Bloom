#include "Bloom/Scene/Entity.h"

#include <ostream>
#include <sstream>

#include <utl/typeinfo.hpp>
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

template <typename ScenePtr, bool IsConst>
std::string EntityHandleEx<ScenePtr, IsConst>::debugDescription() const {
    std::stringstream sstr;
    sstr << "Components: \n";
    forEachComponent([&]<typename T>(utl::tag<T>) {
        if (has<T>()) {
            sstr << utl::nameof<T> << ":\n";
        }
    });
    return sstr.str();
}

template class bloom::EntityHandleEx<Scene*, false>;

template class bloom::EntityHandleEx<Scene const*, true>;

template class bloom::EntityHandleEx<Reference<Scene>, false>;
