#include "Bloom/Application/RenderThread.h"

using namespace bloom;

RenderThread::RenderThread() {
    t = std::thread([this] { worker(); });
}

RenderThread::~RenderThread() {
    std::lock_guard lock(mutex);
    isRunning = false;
}

void RenderThread::submit(std::function<void()> item) {
    std::lock_guard lock(mutex);
    queue.push(std::move(item));
    condvar.notify_one();
}

void RenderThread::worker() {
    std::unique_lock lock(mutex);
    while (isRunning) {
        condvar.wait(lock, [&] { return !queue.empty(); });
        auto f = std::move(queue.front());
        queue.pop();
        lock.unlock();
        f();
        lock.lock();
    }
}
