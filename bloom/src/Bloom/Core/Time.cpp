#include "Bloom/Core/Time.hpp"

using namespace bloom;

Timer::Timer() { reset(); }

void Timer::reset() { watch.reset(); }

void Timer::pause() { watch.pause(); }

void Timer::resume() { watch.resume(); }

void Timer::update() {
    std::size_t const elapsedTimeNS = watch.elapsed_time();

    double const last = ts.absolute;
    ts.absolute = elapsedTimeNS / 1'000'000'000.0;
    ts.delta = ts.absolute - last;

    auto const preciseLast = PreciseTimestep::Duration(preciseTS.absolute);
    preciseTS.absolute = PreciseTimestep::Duration(elapsedTimeNS);
    preciseTS.delta = PreciseTimestep::Duration(preciseTS.absolute.count() -
                                                preciseLast.count());
}
