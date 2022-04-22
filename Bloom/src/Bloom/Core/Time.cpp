#include "Time.hpp"

namespace bloom {
	
	Timer::Timer() {
		reset();
	}
	
	void Timer::reset() {
		watch.reset();
	}
	
	void Timer::update() {
		std::size_t const elapsedTimeNS = watch.elapsed_time();
		double const last = ts.absolute;
		ts.absolute = elapsedTimeNS / 1'000'000'000.0;
		ts.delta = ts.absolute - last;
	}
}
