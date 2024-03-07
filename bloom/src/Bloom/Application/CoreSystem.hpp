#ifndef BLOOM_APPLICATION_CORESYSTEM_H
#define BLOOM_APPLICATION_CORESYSTEM_H

#include "Bloom/Application/MessageSystem.hpp"

namespace bloom {

class Application;

/// Base class of all core systems. Provides messaging capabilities and access
/// to the application
class CoreSystem: public Emitter, public Receiver {
    friend class CoreSystemManager;

public:
    ///
    virtual ~CoreSystem() = default;

    /// \Returns the currently running application
    Application& application() const { return *mApp; }

protected:
    CoreSystem() = default;

private:
    Application* mApp;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_CORESYSTEM_H
