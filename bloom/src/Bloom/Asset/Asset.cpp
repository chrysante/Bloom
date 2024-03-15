#include "Bloom/Asset/Asset.h"

#include <utl/math.hpp>

#include "Bloom/Core/Debug.h"

using namespace bloom;

AssetHandle AssetHandle::generate(AssetType type) {
    return AssetHandle(type, utl::uuid::generate());
}

std::ostream& bloom::operator<<(std::ostream& str, AssetHandle const& handle) {
    return str << handle.ID();
}
