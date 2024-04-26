#include "Poppy/Core/Timestamp.h"

/// This file is touched as a prebuild step to always generate the accurate
/// build time

char const* poppy::buildTimestamp() { return __DATE__ " " __TIME__; }
