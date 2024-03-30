#ifndef BLOOM_GRAPHICS_RENDERGRAPH_NODE_H
#define BLOOM_GRAPHICS_RENDERGRAPH_NODE_H

#include <span>
#include <string>
#include <vector>

#include <range/v3/algorithm.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Graphics/RenderGraph/Interface.h"

namespace bloom {

class RenderContext;

} // namespace bloom

namespace bloom::rg {

using ResourceHandleVariant =
    std::variant<BufferHandle, TextureHandle, std::unique_ptr<SceneGeometry>,
                 std::unique_ptr<SceneLights>>;

class PassInput;

/// Base class for all render graph data nodes
class Resource {
public:
    virtual ~Resource();

    /// \Returns the debug name of this node
    std::string const& name() const { return _name; };

    ResourceHandleVariant const& handle() const { return _handle; }

    std::span<PassInput* const> users() const { return _users; }

protected:
    Resource(std::string name, ResourceHandleVariant handle);

private:
    friend class RenderGraph;

    void addUser(PassInput* user);

    std::string _name;
    ResourceHandleVariant _handle;
    std::vector<PassInput*> _users;
};

///
class UserResource: public Resource {
public:
    UserResource(std::string name, ResourceHandleVariant handle);

private:
};

///
class PassInput {
public:
    explicit PassInput(InterfaceVariant interface);

    ///
    std::string const& name() const { return interface().name(); }

    ///
    InterfaceVariant const& interface() const { return _interface; }

    ///
    Resource* resource() const { return _resource; }

public:
    InterfaceVariant _interface;
    Resource* _resource;
};

///
class PassOutput: public Resource {
public:
    PassOutput(InterfaceVariant interface);

    ///
    InterfaceVariant const& interface() const { return _interface; }

private:
    InterfaceVariant _interface;
};

/// Base class for all render graph pass nodes
class Pass {
public:
    virtual ~Pass() = default;

    /// \Returns the debug name of this node
    std::string const& name() const { return _name; };

    /// Renders this pass
    void execute(RenderContext& ctx) { doExecute(ctx); }

    ///
    PassInput* findInput(std::string_view name);

    ///
    PassOutput* findOutput(std::string_view name);

protected:
    Pass(std::string name): _name(std::move(name)) {}

    ///
    void declareInput(BufferInterface const& interface);
    void declareInput(TextureInterface const& interface);
    void declareInput(SceneGeometryInterface const& interface);
    void declareInput(SceneLightsInterface const& interface);

    ///
    void declareOutput(BufferInterface const& interface);
    void declareOutput(TextureInterface const& interface);

private:
    virtual void doExecute(RenderContext& ctx) = 0;

    std::string _name;
    std::vector<std::unique_ptr<PassInput>> _inputs;
    std::vector<std::unique_ptr<PassOutput>> _outputs;
};

} // namespace bloom::rg

#endif // BLOOM_GRAPHICS_RENDERGRAPH_NODE_H
