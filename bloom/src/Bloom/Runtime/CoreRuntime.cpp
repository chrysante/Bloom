#include "Bloom/Runtime/CoreRuntime.h"

#include <utl/scope_guard.hpp>

#include "Bloom/Application/Application.h"
#include "Bloom/Core/Profile.h"
#include "Bloom/Core/Reference.h"
#include "Bloom/Graphics/Renderer/Renderer.h"
#include "Bloom/Runtime/ScriptSystem.h"
#include "Bloom/Scene/Components/AllComponents.h"
#include "Bloom/Scene/Scene.h"

using namespace bloom;

CoreRuntime::CoreRuntime() = default;

CoreRuntime::~CoreRuntime() { stop(); }

void CoreRuntime::addDelegate(std::shared_ptr<RuntimeDelegate> delegate) {
    std::unique_lock lock(mMutex);
    BL_EXPECT(state() == RuntimeState::Inactive);
    mDelegates.insert(std::move(delegate));
}

void CoreRuntime::removeDelegate(RuntimeDelegate const*) {
    /// This function is very annoying to implement because shared pointers are
    /// not directly comparable to raw pointers. Since we don't need this
    /// function for new we leave it unimplemented.
    BL_UNIMPLEMENTED();
#if 0
    std::unique_lock lock(mMutex);
    BL_EXPECT(mState == RuntimeState::Inactive);
    mDelegates.erase(delegate);
#endif
}

void CoreRuntime::run() {
    std::unique_lock lock(mMutex);
    if (state() == RuntimeState::Running) {
        return;
    }
    if (state() == RuntimeState::Paused) {
        return;
    }
    assert(state() == RuntimeState::Inactive);
    setState(RuntimeState::Running);
    lock.unlock();
    mUpdateThread = std::thread([this] { updateThread(); });
}

void CoreRuntime::stop() {
    std::unique_lock lock(mMutex);
    setState(RuntimeState::Inactive);
    lock.unlock();
    if (mUpdateThread.joinable()) {
        mUpdateThread.join();
    }
}

void CoreRuntime::pause() {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::Running) {
        return;
    }
    mTimer.pause();
    setState(RuntimeState::Paused);
}

void CoreRuntime::resume() {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::Paused) {
        return;
    }
    mTimer.resume();
    setState(RuntimeState::Running);
}

void CoreRuntime::updateThread() {
    static constexpr std::chrono::milliseconds TargetFrameTime{ 10 };
    mTimer.reset();
    for (auto& del: mDelegates) {
        del->start();
    }
    utl::scope_guard onStop = [this] {
        for (auto& del: mDelegates) {
            del->stop();
        }
    };
    while (true) {
        std::unique_lock lock(mMutex);
        switch (mState) {
        case RuntimeState::Running: {
            lock.unlock();
            mTimer.update();
            for (auto& del: mDelegates) {
                del->step(mTimer.timestep());
            }
            std::chrono::nanoseconds remaining =
                TargetFrameTime - mTimer.preciseTimestep().delta;
            if (remaining.count() > 0) {
                std::this_thread::sleep_for(remaining);
            }
            break;
        }
        case RuntimeState::Paused:
            mTimer.pause();
            for (auto& del: mDelegates) {
                del->pause();
            }
            mCV.wait(lock, [&] { return mState != RuntimeState::Paused; });
            if (mState == RuntimeState::Running) {
                mTimer.resume();
                for (auto& del: mDelegates) {
                    del->resume();
                }
            }
            break;
        case RuntimeState::Inactive:
            return;
        }
    }
}

void CoreRuntime::setState(RuntimeState target) {
    mState = target;
    mCV.notify_one();
}
