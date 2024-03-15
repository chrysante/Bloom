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

/// MARK: Modifiers
void CoreRuntime::setDelegate(std::shared_ptr<RuntimeDelegate> delegate) {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::inactive) {
        return;
    }
    mDelegate = std::move(delegate);
}

void CoreRuntime::run() {
    std::unique_lock lock(mMutex);
    if (state() == RuntimeState::running) {
        return;
    }
    if (state() == RuntimeState::paused) {
        return;
    }

    assert(state() == RuntimeState::inactive);

    setState(RuntimeState::running);
    lock.unlock();
    mUpdateThread = std::thread([this] { updateThread(); });
}

void CoreRuntime::stop() {
    std::unique_lock lock(mMutex);
    setState(RuntimeState::inactive);
    lock.unlock();
    if (mUpdateThread.joinable()) {
        mUpdateThread.join();
    }
}

void CoreRuntime::pause() {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::running) {
        return;
    }
    mTimer.pause();
    setState(RuntimeState::paused);
}

void CoreRuntime::resume() {
    std::unique_lock lock(mMutex);
    if (mState != RuntimeState::paused) {
        return;
    }
    mTimer.resume();
    setState(RuntimeState::running);
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
        case RuntimeState::running:
            lock.unlock();
            mTimer.update();
            if (mDelegate) {
                mDelegate->step(Timestep{});
            }
            break;
        case RuntimeState::paused:
            if (mDelegate) {
                mDelegate->pause();
            }
            mCV.wait(lock, [&] { return mState != RuntimeState::paused; });
            if (mState == RuntimeState::running && mDelegate) {
                mDelegate->resume();
            }
            break;

        case RuntimeState::inactive:
            return;
        }
    }
}

void CoreRuntime::setState(RuntimeState target) {
    mState = target;
    mCV.notify_one();
}
