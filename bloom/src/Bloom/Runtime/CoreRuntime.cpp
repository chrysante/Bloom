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

CoreRuntime::CoreRuntime(std::shared_ptr<RuntimeDelegate> delegate):
    mDelegate(std::move(delegate)) {}

CoreRuntime::~CoreRuntime() { stop(); }

bool CoreRuntime::setDelegate(std::shared_ptr<RuntimeDelegate> delegate) {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::Inactive) {
        return false;
    }
    mDelegate = std::move(delegate);
    return true;
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

/// MARK: Private
void CoreRuntime::updateThread() {
    mTimer.reset();
    if (mDelegate) {
        mDelegate->start();
    }
    utl::scope_guard onStop = [this] {
        if (mDelegate) {
            mDelegate->stop();
        }
    };
    while (true) {
        std::unique_lock lock(mMutex);
        switch (mState) {
        case RuntimeState::Running:
            lock.unlock();
            mTimer.update();
            if (mDelegate) {
                mDelegate->step(Timestep{});
            }
            break;
        case RuntimeState::Paused:
            if (mDelegate) {
                mDelegate->pause();
            }
            mCV.wait(lock, [&] { return mState != RuntimeState::Paused; });
            if (mState == RuntimeState::Running && mDelegate) {
                mDelegate->resume();
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
