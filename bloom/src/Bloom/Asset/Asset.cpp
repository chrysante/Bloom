#include "Bloom/Asset/Asset.h"

#include <utl/math.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Core/Debug.h"

using namespace bloom;

AssetHandle AssetHandle::generate(AssetType type) {
    return AssetHandle(type, utl::uuid::generate());
}

std::ostream& bloom::operator<<(std::ostream& str, AssetHandle const& handle) {
    return str << handle.ID();
}

Reference<Asset> bloom::getAsset(AssetHandle handle, AssetRepresentation repr) {
    Reference<Asset> ref;
    Application::DispatchMessage(DispatchToken::Now,
                                 AMGetAsset{ .handle = handle,
                                             .asset = &ref,
                                             .representation = repr });
    return ref;
}
