#pragma once

#include "Window.hpp"
#include "WindowDelegate.hpp"
#include "CoreSystemManager.hpp"
#include "MessageSystem.hpp"
#include "Commands.hpp"

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Time.hpp"

#include <utl/utility.hpp>
#include <memory>

BLOOM_API int main(int, char const**, char const**);

namespace bloom {
	
	class Application;
	class HardwareDevice;
	
	BLOOM_WEAK_API Application* createApplication();
	
	struct BLOOM_API CommandLineArgs {
		int argc;
		char const** argv;
		char const** env;
	};
	
	struct BLOOM_API FrameData {
		std::size_t frameCount;
		
		std::chrono::nanoseconds time;
		std::chrono::nanoseconds deltaTime;
	};
	
	class BLOOM_API Application:
		private MessageSystem,
		protected Reciever,
		protected Emitter
	{
		friend int ::main(int, char const**, char const**);
	public:
		Application();
		~Application();
		
		/// MARK: Virtual Interface
		virtual void init() {}
		virtual void shutdown() {}
		virtual void frame() {}
		
		/// MARK: Queries
		CommandLineArgs commandLineArgs() const { return cmdLineArgs; }
		HardwareDevice& device() { return coreSystems().device(); }
		CoreSystemManager& coreSystems() { return mCoreSystems; }
		Timestep time() const { return mTimer.timestep(); }
		
		utl::small_vector<Window*> getWindows();
		auto windows() {
			return utl::transform_range(mWindows, [](auto&& p) -> auto& { return *p.first.get(); });
		}
		using MessageSystem::makeReciever;
		using MessageSystem::makeEmitter;
		
		
		/// MARK: Modifiers
		Window& createWindow(WindowDescription const&, std::unique_ptr<WindowDelegate> = nullptr);
		Window& createWindow(WindowDescription const&, WindowDelegate*);
		
	private:
		/// MARK: Private
		void run();
		
		void doInit();
		void registerListeners();
		
		void doShutdown();

		void doFrame();
		void clearClosingWindows();
		
	private:
		CommandLineArgs cmdLineArgs{};
		utl::vector<std::pair<std::unique_ptr<bloom::Window>,
							  std::unique_ptr<bloom::WindowDelegate>>> mWindows;
		
		CoreSystemManager mCoreSystems;
		Timer mTimer;
		
		std::size_t mFrameCounter = 0;
	};
	
}
