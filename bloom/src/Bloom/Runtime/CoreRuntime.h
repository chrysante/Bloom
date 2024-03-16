#ifndef BLOOM_RUNTIME_CORERUNTIME_H
#define BLOOM_RUNTIME_CORERUNTIME_H

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <utl/hashtable.hpp>

#include "Bloom/Application/CoreSystem.h"
#include "Bloom/Core/Base.h"
#include "Bloom/Core/Time.h"

namespace bloom {

struct UpdateOptions {
    std::size_t stepsPerSecond = 50;
};

enum class RuntimeState { Inactive = 0, Paused = 1, Running = 2 };

/// Callback class to customize the runtime behaviour
class BLOOM_API RuntimeDelegate {
public:
    virtual ~RuntimeDelegate() = default;

    ///
    virtual void start(){};

    ///
    virtual void stop(){};

    ///
    virtual void pause(){};

    ///
    virtual void resume(){};

    ///
    virtual void step(Timestep){};
};

/// Manages the update thread
class BLOOM_API CoreRuntime: public CoreSystem {
public:
    explicit CoreRuntime();

    ~CoreRuntime();

    ///
    void init();

    ///
    UpdateOptions updateOptions() const { return mUpdateOptions; }

    ///
    RuntimeState state() const { return mState; }

    /// \pre `state()` must be `Inactive`
    void addDelegate(std::shared_ptr<RuntimeDelegate> delegate);

    /// \pre `state()` must be `Inactive`
    void removeDelegate(RuntimeDelegate const* delegate);

    ///
    void setUpdateOptions(UpdateOptions options) { mUpdateOptions = options; }

    ///
    void run();

    ///
    void stop();

    ///
    void pause();

    ///
    void resume();

private:
    void updateThread();
    void setState(RuntimeState target);

    std::thread mUpdateThread;
    std::mutex mMutex;
    std::condition_variable mCV;

    RuntimeState mState = RuntimeState::Inactive;
    Timer mTimer;
    UpdateOptions mUpdateOptions;

    utl::hashset<std::shared_ptr<RuntimeDelegate>> mDelegates;
};

} // namespace bloom

#endif // BLOOM_RUNTIME_CORERUNTIME_H
