#include <catch2/catch_test_macros.hpp>

#include <utl/utility.hpp>

#include "Bloom/Runtime/CoreRuntime.h"

using namespace bloom;

namespace {

struct Delegate: RuntimeDelegate {
    void start() override { value = 1; }
    void step(Timestep) override {
        utl::busy_wait([&] { return mayStep.load(); });
        value = 2;
    }
    void pause() override { value = 3; }
    void resume() override { value = 4; }
    void stop() override { value = 5; }

    std::atomic_bool mayStep = false;
    std::atomic_int value = 0;
};

} // namespace

TEST_CASE("RuntimeSystem create/destroy") {
    CoreRuntime crt;
    CHECK(crt.state() == RuntimeState::Inactive);
}

TEST_CASE("RuntimeSystem") {
    auto del = std::make_shared<Delegate>();
    CoreRuntime crt;
    crt.addDelegate(del);
    CHECK(crt.state() == RuntimeState::Inactive);
    CHECK(del->value == 0);

    del->mayStep = false;
    crt.run();
    CHECK(crt.state() == RuntimeState::Running);
    utl::busy_wait([&] { return del->value == 1; });
    del->mayStep = true;
    utl::busy_wait([&] { return del->value == 2; });
    CHECK(del->value == 2);

    crt.pause();
    CHECK(crt.state() == RuntimeState::Paused);
    utl::busy_wait([&] { return del->value == 3; });
    CHECK(del->value == 3);

    del->mayStep = false;
    crt.resume();
    CHECK(crt.state() == RuntimeState::Running);
    utl::busy_wait([&] { return del->value == 4; });
    CHECK(del->value == 4);
    del->mayStep = true;
    utl::busy_wait([&] { return del->value == 2; });
    CHECK(del->value == 2);

    crt.stop();
    CHECK(crt.state() == RuntimeState::Inactive);
    utl::busy_wait([&] { return del->value == 5; });
    CHECK(del->value == 5);
}
