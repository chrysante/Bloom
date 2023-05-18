#pragma once

#include <concepts>
#include <memory>
#include <utl/common.hpp>

#include "Base.hpp"
#include "Debug.hpp"

namespace bloom {

template <typename T>
using Reference = std::shared_ptr<T>;

template <typename T>
using WeakReference = std::weak_ptr<T>;

template <typename Derived, typename Base>
    requires std::derived_from<Derived, Base>
Reference<Derived> as(Reference<Base> const& baseRef) {
    assert(baseRef == nullptr ||
           dynamic_cast<Derived*>(baseRef.get()) != nullptr);
    return std::static_pointer_cast<Derived>(baseRef);
}

template <typename Derived, typename Base>
    requires std::derived_from<Derived, Base>
Reference<Derived> as(Reference<Base>&& baseRef) {
    assert(baseRef == nullptr ||
           dynamic_cast<Derived*>(baseRef.get()) != nullptr);
    return std::static_pointer_cast<Derived>(std::move(baseRef));
}

template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
Reference<T> allocateRef(Args&&... args) {
    return std::make_shared<T>(UTL_FORWARD(args)...);
}

} // namespace bloom
