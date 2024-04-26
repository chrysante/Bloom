#ifndef BLOOM_GRAPHICS_RENDERGRAPH_PASSREGISTRY_H
#define BLOOM_GRAPHICS_RENDERGRAPH_PASSREGISTRY_H

#include <functional>
#include <memory>
#include <span>
#include <string>

#include <utl/common.hpp>
#include <utl/hashtable.hpp>

#include "Bloom/Graphics/RenderGraph/Fwd.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

/// Registers a derived pass with the registry. This is meant to be invoked at
/// file scope in the pass implementation file
#define BL_REGISTER_PASS(PassType, StaticName)                                 \
    UTL_STATIC_INIT {                                                          \
        ::bloom::rg::internal::doRegisterPass(                                 \
            ::bloom::rg::internal::makeEntry<PassType>(StaticName));           \
    }

namespace bloom::rg {

/// Entry in the pass registry. Holds a vtable with all relevant information
/// about a pass
struct PassRegistryEntry {
    struct VTable {
        template <typename T>
        using FnPtr = std::add_pointer_t<T>;

        FnPtr<std::unique_ptr<Pass>(std::string name)> make;
        FnPtr<std::span<InterfaceVariant const>()> sinks;
        FnPtr<std::span<InterfaceVariant const>()> sources;
    };

    std::string name;
    VTable vtable;
};

/// Static class that stores vtables for derived pass classes
class PassRegistry {
public:
    /// \Returns a view over all registered passes
    static std::span<PassRegistryEntry const> entries();
};

namespace internal {

/// Adds an entry to the pass registry
void doRegisterPass(PassRegistryEntry entry);

template <typename PassType>
static PassRegistryEntry makeEntry(std::string name) {
    // clang-format off
    return {
        .name = std::move(name),
        .vtable = {
            .make = [](std::string name) -> std::unique_ptr<Pass> {
                return std::make_unique<PassType>(std::move(name));
            },
            .sinks = PassType::Sinks,
            .sources = PassType::Sources
        }
    }; // clang-format on
}

} // namespace internal

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_PASSREGISTRY_H
