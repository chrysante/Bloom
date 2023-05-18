#pragma once

#include <utl/stopwatch.hpp>

#include "Bloom/Core/Base.hpp"

namespace bloom {

struct BLOOM_API Timestep {
    double absolute, delta;
};

struct BLOOM_API PreciseTimestep {
    using Duration = utl::precise_stopwatch::duration;
    Duration absolute, delta;
};

class BLOOM_API Timer {
public:
    Timer();
    void reset();
    void pause();
    void resume();
    void update();
    Timestep timestep() const { return ts; }
    PreciseTimestep preciseTimestep() const { return preciseTS; }

private:
    utl::precise_stopwatch watch;
    Timestep ts;
    PreciseTimestep preciseTS;
};

} // namespace bloom
