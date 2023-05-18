#include "Bloom/Core/Profile.hpp"

namespace bloom {

void Profile::add(std::string const& name, ProfileResult const& value) {
    _profiles[name] = value;
}

Profile Profilers::update{};

ProfileHandle::ProfileHandle(Profile* p, std::string name):
    name(std::move(name)),
    profile(p),
    begin(std::chrono::high_resolution_clock::now()) {}

ProfileHandle::~ProfileHandle() {
    auto const result = finalize();
    profile->add(name, result);
}

ProfileResult ProfileHandle::finalize() {
    auto const end = std::chrono::high_resolution_clock::now();
    ProfileResult result;
    result.duration = end - begin;
    return result;
}

} // namespace bloom
