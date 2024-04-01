#ifndef BLOOM_CORE_DYNCAST_H
#define BLOOM_CORE_DYNCAST_H

#include <concepts>
#include <memory>

#include <utl/dyncast.hpp>

namespace bloom {

using utl::cast;
using utl::dyncast;
using utl::isa;
using utl::visit;

template <typename T>
using DynUniquePtr = std::unique_ptr<T, utl::dyn_deleter>;

template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
DynUniquePtr<T> allocate(Args&&... args) {
    return DynUniquePtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace bloom

#endif // BLOOM_CORE_DYNCAST_H
