#include "Bloom/Asset/Asset.hpp"

#include <utl/math.hpp>

#include "Bloom/Core/Debug.hpp"

using namespace bloom;

AssetHandle AssetHandle::generate(AssetType type) {
    return AssetHandle(type, utl::uuid::generate());
}

std::ostream& bloom::operator<<(std::ostream& str, AssetHandle const& handle) {
    return str << handle.ID();
}
