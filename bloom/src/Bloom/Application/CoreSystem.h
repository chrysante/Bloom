#ifndef BLOOM_APPLICATION_CORESYSTEM_H
#define BLOOM_APPLICATION_CORESYSTEM_H

#include "Bloom/Application/MessageSystem.h"

namespace bloom {

class Application;

/// Base class of all core systems. Provides messaging capabilities and access
/// to the application
class CoreSystem: public Emitter, public Receiver {
public:
    /// Systems are polymorphic and thus not copyable
    /// @{
    CoreSystem(CoreSystem const&) = delete;
    CoreSystem& operator=(CoreSystem const&) = delete;
    /// @}

    ///
    virtual ~CoreSystem() = default;

    /// \Returns the currently running application
    Application& application() const { return *mApp; }

protected:
    CoreSystem() = default;

private:
    friend class CoreSystemManager;

    /// Called after `mApp` is set
    virtual void init() {}

    Application* mApp;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_CORESYSTEM_H
