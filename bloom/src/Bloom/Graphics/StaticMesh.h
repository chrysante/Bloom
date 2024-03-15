#ifndef BLOOM_GRAPHICS_STATICMESH_H
#define BLOOM_GRAPHICS_STATICMESH_H

#include <utl/vector.hpp>

#include "Bloom/Core/Core.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/Graphics/Mesh.h"
#include "Bloom/Graphics/Vertex.h"

namespace bloom {

struct StaticMeshData;
class StaticMeshRenderer;

class BLOOM_API StaticMesh: public Mesh {
    friend class AssetManager;

public:
    BL_DEFINE_ASSET_CTOR(StaticMesh, Mesh)

    Reference<StaticMeshData> getData() { return mData; }
    Reference<StaticMeshRenderer> getRenderer() { return mRenderer; }

private:
    Reference<StaticMeshData> mData;
    Reference<StaticMeshRenderer> mRenderer;
};

struct StaticMeshData {
    utl::vector<Vertex3D> vertices;
    utl::vector<std::uint32_t> indices;
};

class BLOOM_API StaticMeshRenderer {
    friend class AssetManager;

public:
    BufferView vertexBuffer() const { return mVertexBuffer; }
    BufferView indexBuffer() const { return mIndexBuffer; }

private:
    BufferHandle mVertexBuffer, mIndexBuffer;
};

} // namespace bloom

#endif // BLOOM_GRAPHICS_STATICMESH_H
