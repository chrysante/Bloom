#ifndef BLOOM_GRAPHICS_RENDERGRAPH_NODE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_NODE_H

#include <span>
#include <string>
#include <vector>

#include <range/v3/algorithm.hpp>
#include <utl/common.hpp>

#include "Bloom/Core/Debug.h"

namespace bloom {

class RGResourceNode;

///
struct RGResource {
    /// The resource once it's allocated
    RGResourceNode* node = nullptr;
};

///
enum class RGBufferNodeFlags { Dynamic };

UTL_ENUM_OPERATORS(RGBufferNodeFlags)

///
struct RGBufferDescription {
    std::string name;
    RGBufferNodeFlags flags;
};

/// Base class for all render graph pass nodes
class RGPassNode {
public:
    virtual ~RGPassNode() = default;

    /// \Returns the debug name of this node
    std::string const& name() const { return _name; };

    /// Renders this pass
    void execute() { doExecute(); }

    ///
    std::span<RGResource const> inputs() const { return _inputs; }

    ///
    std::span<RGResource const> outputs() const { return _outputs; }

protected:
    RGPassNode(std::string name): _name(std::move(name)) {}

    ///
    void declareInput(RGBufferDescription const& desc);

    ///
    void declareOutput(RGBufferDescription const& desc);

private:
    virtual void doExecute() = 0;

    std::string _name;
    std::vector<RGResource> _inputs;
    std::vector<RGResource> _outputs;
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
