#ifndef BLOOM_GRAPHICS_RENDERGRAPH_NODE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_NODE_H

#include <span>
#include <string>
#include <variant>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <utl/common.hpp>

#include "Bloom/Core/Debug.h"

namespace bloom {

class RenderContext;
class BufferHandle;
class TextureHandle;
class SamplerHandle;
class DepthStencilHandle;
class RGResourceNode;
class RenderScene;

///
enum class RGBufferNodeFlags { Dynamic };

UTL_ENUM_OPERATORS(RGBufferNodeFlags)

///
struct RGBufferDescription {
    std::string name;
    RGBufferNodeFlags flags;
    BufferHandle* handle = nullptr;
};

struct RGTextureDescription {
    std::string name;
    TextureHandle* handle = nullptr;
};

struct RGSceneDescription {
    std::string name;
    RenderScene** handle;
};

///
struct RGResource {
    /// The resource once it's allocated
    RGResourceNode* node = nullptr;

    /// The resource descriptor
    std::variant<RGBufferDescription, RGTextureDescription, RGSceneDescription>
        desc;
};

/// Base class for all render graph pass nodes
class RGPassNode {
public:
    virtual ~RGPassNode() = default;

    /// \Returns the debug name of this node
    std::string const& name() const { return _name; };

    /// Renders this pass
    void execute(RenderContext& ctx) { doExecute(ctx); }

    ///
    RGResource* findInput(std::string_view name);

    ///
    RGResource* findOutput(std::string_view name);

protected:
    RGPassNode(std::string name): _name(std::move(name)) {}

    ///
    void declareInput(RGBufferDescription const& desc);
    void declareInput(RGTextureDescription const& desc);
    void declareInput(RGSceneDescription const& desc);

    ///
    void declareOutput(RGBufferDescription const& desc);
    void declareOutput(RGTextureDescription const& desc);

private:
    virtual void doExecute(RenderContext& ctx) = 0;

    std::string _name;
    std::vector<std::unique_ptr<RGResource>> _inputs;
    std::vector<std::unique_ptr<RGResource>> _outputs;
};

/// Base class for all render graph data nodes
class RGResourceNode {
public:
    virtual ~RGResourceNode() = default;

    /// \Returns the debug name of this node
    std::string const& name() const { return _name; };

protected:
    RGResourceNode(std::string name): _name(std::move(name)) {}

private:
    std::string _name;
};

/// Generic buffer
class RGBufferNode: public RGResourceNode {};

} // namespace bloom

#endif // BLOOM_GRAPHICS_RENDERGRAPH_NODE_H
