#pragma once

#include <concepts>

#include <utl/functional.hpp>

#include "Bloom/Core/Base.hpp"

namespace bloom {

struct BLOOM_API CustomCommand {
    CustomCommand(std::invocable auto&& block): function(UTL_FORWARD(block)) {}
    utl::function<void()> function;
};

} // namespace bloom
