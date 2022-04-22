#pragma once

#include "Bloom/Core/Base.hpp"
#include <utl/stopwatch.hpp>

namespace bloom {
	
	struct BLOOM_API TimeStep {
		double absolute, delta;
	};
	
	class BLOOM_API Timer {
	public:
		Timer();
		void reset();
		void update();
		TimeStep getTimeStep() const { return ts; }
	private:
		utl::precise_stopwatch watch;
		TimeStep ts;
	};
	
}
