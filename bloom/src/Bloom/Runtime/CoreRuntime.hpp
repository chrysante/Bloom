#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "Bloom/Application/CoreSystem.hpp"
#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"

namespace bloom {

struct BLOOM_API UpdateOptions {
    std::size_t stepsPerSecond = 50;
};

enum class BLOOM_API RuntimeState { inactive = 0, paused = 1, running = 2 };

class BLOOM_API RuntimeDelegate {
public:
    virtual void start(){};
    virtual void stop(){};
    virtual void pause(){};
    virtual void resume(){};
    virtual void step(Timestep){};
};

class BLOOM_API CoreRuntime: public CoreSystem {
public:
    CoreRuntime() = default;
    explicit CoreRuntime(std::shared_ptr<RuntimeDelegate>);

    ~CoreRuntime();

    void init();

    /// MARK: Queries
    UpdateOptions updateOptions() const { return mUpdateOptions; }
    RuntimeState state() const { return mState; }

    /// MARK: Modifiers
    void setDelegate(std::shared_ptr<RuntimeDelegate>);
    void setUpdateOptions(UpdateOptions options) { mUpdateOptions = options; }

    void run();
    void stop();
    void pause();
    void resume();

private:
    /// MARK: Private
    void updateThread();
    void setState(RuntimeState target);

private:
    std::thread mUpdateThread;
    std::mutex mMutex;
    std::condition_variable mCV;

    RuntimeState mState = RuntimeState::inactive;
    Timer mTimer;
    UpdateOptions mUpdateOptions;

    std::shared_ptr<RuntimeDelegate> mDelegate;
};

} // namespace bloom
