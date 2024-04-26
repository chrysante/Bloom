#ifndef BLOOM_APPLICATION_RENDERTHREAD_H
#define BLOOM_APPLICATION_RENDERTHREAD_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace bloom {

/// Wraps an `std::thread` and provides methods to push work and request
/// rendering.
///
/// This can probably be generalized to `WorkerThread` or something because
/// nothing in this class is rendering specific but we leave it like this for
/// now
class RenderThread {
public:
    RenderThread();

    ~RenderThread();

    /// Submits \p item to be executed on the render thread
    void submit(std::function<void()> item);

private:
    void worker();

    std::queue<std::function<void()>> queue;
    std::condition_variable condvar;
    std::mutex mutex;
    std::thread t;
    bool isRunning = true;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_RENDERTHREAD_H
