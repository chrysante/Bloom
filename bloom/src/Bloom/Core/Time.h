#ifndef BLOOM_CORE_TIME_H
#define BLOOM_CORE_TIME_H

#include <chrono>

#include <utl/stopwatch.hpp>

#include "Bloom/Core/Base.h"

namespace bloom {

/// Absolute and delta time in seconds represented by doubles
struct Timestep {
    double absolute;
    double delta;
};

/// Absolute and delta time in nanoseconds represented by
/// `std::chrono::nanoseconds`
struct PreciseTimestep {
    using Duration = std::chrono::nanoseconds;
    Duration absolute;
    Duration delta;
};

/// Precise timer used to time frames
class BLOOM_API Timer {
    static_assert(std::same_as<utl::precise_stopwatch::duration,
                               PreciseTimestep::Duration>);

public:
    /// Constructs and starts the timer
    Timer();

    /// Resets the absolute time to zero
    void reset();

    /// Pauses the absolute time
    void pause();

    /// Continues the absolute time
    void resume();

    /// This must be called each frame before accessing the timestep
    void update();

    /// \Returns the imprecise timestep
    Timestep timestep() const { return ts; }

    /// \Returns the precise timestep
    PreciseTimestep preciseTimestep() const { return preciseTS; }

private:
    utl::precise_stopwatch watch;
    Timestep ts;
    PreciseTimestep preciseTS;
};

} // namespace bloom

#endif // BLOOM_CORE_TIME_H
