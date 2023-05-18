#pragma once

#include <chrono>
#include <string>
#include <utl/common.hpp>
#include <utl/hashmap.hpp>

#include "Bloom/Core/Base.hpp"

#define BLOOM_PROFILE_IMPL(PROFILE, NAME)                                      \
    auto const UTL_UNIQUE_NAME(_profile_var_) =                                \
        ::bloom::ProfileHandle(PROFILE, NAME)

#define BLOOM_PROFILE(STAGE, ...)                                              \
    UTL_VFUNC(BLOOM_PROFILE_, STAGE __VA_OPT__(, ) __VA_ARGS__)

#define BLOOM_PROFILE_1(STAGE)                                                 \
    BLOOM_PROFILE_IMPL(&::bloom::Profilers::STAGE, __PRETTY_FUNCTION__)
#define BLOOM_PROFILE_2(STAGE, NAME)                                           \
    BLOOM_PROFILE_IMPL(&::bloom::Profilers::STAGE, NAME)

namespace bloom {

struct ProfileResult {
    std::chrono::high_resolution_clock::duration duration;
};

class BLOOM_API Profile {
public:
    void add(std::string const&, ProfileResult const&);

    utl::hashmap<std::string, ProfileResult> const& get() const {
        return _profiles;
    }

private:
    utl::hashmap<std::string, ProfileResult> _profiles;
};

struct BLOOM_API Profilers {
    static Profile update;
};

class BLOOM_API ProfileHandle {
public:
    ProfileHandle(Profile*, std::string name);
    ~ProfileHandle();

private:
    ProfileResult finalize();

private:
    std::string name;
    Profile* profile;
    std::chrono::high_resolution_clock::time_point begin;
};

struct Profiles {
    static bloom::Profile frame;
    static bloom::Profile init;
};

} // namespace bloom
