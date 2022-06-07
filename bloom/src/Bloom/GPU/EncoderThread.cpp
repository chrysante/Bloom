#include "EncoderThread.hpp"

#include "Bloom/Core/Core.hpp"

namespace bloom {

	RenderThread::RenderThread(RenderThreadDescription const& d):
		desc(d),
		frameSemaphore(desc.numFramesInFlight)
	{
		bloomExpect(desc.numFramesInFlight >= 2);
		bloomExpect(desc.numFramesInFlight <= 3);
		
		
	}
	
	RenderThread::~RenderThread() {
		stop();
	}
	
	void RenderThread::execute() {
		isRunning = true;
		renderThread = std::thread(&RenderThread::renderLoop, this);
	}
	
	void RenderThread::stop() {
		isRunning = false;
		if (renderThread.joinable()) {
			renderThread.join();
		}
	}
	
	void RenderThread::beginFrame() noexcept {
		
		std::unique_lock lock
		
		
		
		
		
		
		
		
	}
	
	void RenderThread::endFrame() noexcept {
		currentFrame = (currentFrame + 1) % desc.numFramesInFlight;
		frameMutex.unlock();
	}
	
	void RenderThread::submit(utl::function<void()> fn) noexcept {
		frames[currentFrame].workItems.push_back(std::move(fn));
	}
	
	void RenderThread::setWantsDisplay() {
		
	}
	
	void RenderThread::renderLoop() {
		while (isRunning) {
			frameSemaphore.acquire();
		}
	}
	
}

