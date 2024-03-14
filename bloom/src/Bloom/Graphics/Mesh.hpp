#ifndef BLOOM_GRAPHICS_MESH_H
#define BLOOM_GRAPHICS_MESH_H

#include "Bloom/Asset/Asset.hpp"

namespace bloom {

/// Base class of `StaticMesh` and `SkeletalMesh`
class BLOOM_API Mesh: public Asset {
protected:
    using Asset::Asset;
};

/// Stub implementation
class BLOOM_API SkeletalMesh: public Mesh {
public:
    BL_DEFINE_ASSET_CTOR(SkeletalMesh, Mesh)
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_MESH_H
