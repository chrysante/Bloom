#include "Bloom/Application/RenderThread.h"

#ifdef __GNUC__
#include <pthread.h>
#endif

using namespace bloom;

RenderThread::RenderThread() {
    t = std::thread([this] {
#ifdef __GNUC__
        pthread_setname_np("RenderThread");
#endif
        worker();
    });
}

RenderThread::~RenderThread() {
    mutex.lock();
    isRunning = false;
    mutex.unlock();
    condvar.notify_one();
    t.join();
}

void RenderThread::submit(std::function<void()> item) {
    std::lock_guard lock(mutex);
    queue.push(std::move(item));
    condvar.notify_one();
}

void RenderThread::worker() {
    std::unique_lock lock(mutex);
    while (true) {
        condvar.wait(lock, [&] { return !queue.empty() || !isRunning; });
        if (!isRunning) {
            return;
        }
        auto f = std::move(queue.front());
        queue.pop();
        lock.unlock();
        f();
        lock.lock();
    }
}
