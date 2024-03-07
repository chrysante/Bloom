#include <Catch2/Catch2.hpp>

#include "Bloom/Runtime/CoreRuntime.hpp"
#include <utl/utility.hpp>

using namespace bloom;

namespace {
struct Delegate: RuntimeDelegate {
    void start() override { value = 1; }
    void step(Timestep) override { value = 2; }
    void pause() override { value = 3; }
    void resume() override { value = 4; }
    void stop() override { value = 5; }
    std::atomic_int value = 0;
};
} // namespace

TEST_CASE("RuntimeSystem create/destroy") {
    CoreRuntime crt;
    CHECK(crt.state() == RuntimeState::inactive);
}

TEST_CASE("RuntimeSystem") {
    auto del = std::make_shared<Delegate>();
    CoreRuntime crt(del);
    CHECK(crt.state() == RuntimeState::inactive);
    CHECK(del->value == 0);

    crt.run();
    CHECK(crt.state() == RuntimeState::running);
    utl::busy_wait([&] { return del->value >= 1; });
    utl::busy_wait([&] { return del->value == 2; });
    CHECK(del->value == 2);

    crt.pause();
    CHECK(crt.state() == RuntimeState::paused);
    utl::busy_wait([&] { return del->value == 3; });
    CHECK(del->value == 3);

    crt.resume();
    CHECK(crt.state() == RuntimeState::running);
    utl::busy_wait([&] { return del->value == 4; });

    crt.stop();
    CHECK(crt.state() == RuntimeState::inactive);
    utl::busy_wait([&] { return del->value == 5; });
    CHECK(del->value == 5);
}
