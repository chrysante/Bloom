#pragma once

#include "MessageSystem.hpp"

namespace bloom {
	
	class Application;

	class CoreSystem: public Emitter, public Reciever {
		friend class CoreSystemManager;
		
	public:
		virtual ~CoreSystem() = default;
		Application& application() const { return *mApp; }
		
	private:
		Application* mApp;
	};
	
}
