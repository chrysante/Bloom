#ifndef BLOOM_CORE_REFERENCE_H
#define BLOOM_CORE_REFERENCE_H

#include <concepts>
#include <memory>
#include <utl/common.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Dyncast.h"

namespace bloom {

template <typename T>
using Reference = std::shared_ptr<T>;

template <typename T>
using WeakReference = std::weak_ptr<T>;

template <typename Derived, typename Base>
    requires std::derived_from<Derived, Base>
Reference<Derived> as(Reference<Base> const& base) {
    if constexpr (std::is_polymorphic_v<Base>) {
        return std::dynamic_pointer_cast<Derived>(base);
    }
    else if constexpr (utl::dc::Dynamic<Base>) {
        if (auto* p = dyncast<Derived*>(base.get())) {
            return Reference<Derived>(base, p);
        }
        return nullptr;
    }
    else {
        return std::static_pointer_cast<Derived>(base);
    }
}

template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
Reference<T> allocateRef(Args&&... args) {
    return std::make_shared<T>(UTL_FORWARD(args)...);
}

} // namespace bloom

#endif // BLOOM_CORE_REFERENCE_H
