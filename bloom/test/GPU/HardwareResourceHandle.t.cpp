#include <catch2/catch_test_macros.hpp>

#include "Bloom/GPU/HardwareResourceHandle.h"

using namespace bloom;

namespace {

void deleter(void* p) { ++*(int*)p; }

struct IntHandle: HardwareResourceHandle {
    IntHandle(void* native): HardwareResourceHandle(native, deleter) {}
};

} // namespace

TEST_CASE("Release", "[HardwareResourceHandle]") {
    int resource = 0;
    IntHandle h(&resource);
    CHECK(resource == 0);
    h.release();
    CHECK(resource == 1);
}

TEST_CASE("Copy constructor", "[HardwareResourceHandle]") {
    int resource = 0;
    IntHandle h(&resource);
    auto i = h;
    h.release();
    CHECK(resource == 0);
    i.release();
    CHECK(resource == 1);
}

TEST_CASE("Copy assignment", "[HardwareResourceHandle]") {
    int resource = 0, other = 0;
    IntHandle h(&resource);
    IntHandle i(&other);
    CHECK(other == 0);
    i = h;
    CHECK(other == 1);
    h.release();
    CHECK(resource == 0);
    i.release();
    CHECK(resource == 1);
}

TEST_CASE("Move constructor", "[HardwareResourceHandle]") {
    int resource = 0;
    IntHandle h(&resource);
    auto i = std::move(h);
    CHECK(h == nullptr);
    CHECK(resource == 0);
    i.release();
    CHECK(resource == 1);
}

TEST_CASE("Move assignment", "[HardwareResourceHandle]") {
    int resource = 0, other = 0;
    IntHandle h(&resource);
    IntHandle i(&other);
    CHECK(other == 0);
    i = std::move(h);
    CHECK(other == 1);
    CHECK(h == nullptr);
    CHECK(resource == 0);
    i.release();
    CHECK(resource == 1);
}
