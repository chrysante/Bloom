#include "Bloom/Graphics/RenderGraph/PassRegistry.h"

#include "Bloom/Core/Debug.h"

using namespace bloom;

namespace {

struct Hash {
    size_t operator()(PassRegistryEntry const& entry) const {
        return std::hash<std::string>{}(entry.name);
    }
};

struct Equal {
    bool operator()(PassRegistryEntry const& lhs,
                    PassRegistryEntry const& rhs) const {
        return lhs.name == rhs.name;
    }
};

struct PassRegistryImpl: PassRegistry {
    utl::hashset<PassRegistryEntry, Hash, Equal> _entries;
};

} // namespace

static PassRegistryImpl& instance() {
    static PassRegistryImpl inst;
    return inst;
}

std::span<PassRegistryEntry const> PassRegistry::entries() {
    return instance()._entries.values();
}

void internal::doRegisterPass(PassRegistryEntry entry) {
    bool success = instance()._entries.insert(entry).second;
    BL_ASSERT(success, "Failed to register pass. Name already exists");
}
