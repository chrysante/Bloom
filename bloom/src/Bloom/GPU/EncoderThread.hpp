#pragma once

#include <utl/functional.hpp>
#include <utl/vector.hpp>
#include <array>
#include <mutex>
#include <semaphore>
#include <thread>

namespace bloom {
	
	struct RenderThreadDescription {
		std::size_t numFramesInFlight = 3;
	};
	
	class RenderThread {
	public:
		RenderThread(RenderThreadDescription const& = {});
		~RenderThread();
		
		void execute();
		void stop();
		
		void beginFrame() noexcept;
		void endFrame() noexcept;
		void submit(utl::function<void()>) noexcept;
		
		void setWantsDisplay();
		
	private:
		void renderLoop();
		
	private:
		struct FrameData {
			utl::vector<utl::function<void()>> workItems;
		};
	
	private:
		RenderThreadDescription desc;
		
		std::array<FrameData, 3> frames;
		std::size_t currentWriteFrame = 0;
		std::size_t currentReadFrame = 0;
		
		std::mutex frameMutex;
		std::condition_variable frameCV;
		
		
		
		std::atomic_bool isRunning = false;
		std::thread renderThread;
		
		std::counting_semaphore<3> frameSemaphore;
	};
	
}
