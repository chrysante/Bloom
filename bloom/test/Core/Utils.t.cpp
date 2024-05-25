#include <catch2/catch_test_macros.hpp>

#include "Bloom/Core/Utils.h"

using namespace bloom;

static_assert(std::input_iterator<CStringIterator<char const>>);
static_assert(std::ranges::range<CStringIterator<char const>>);

TEST_CASE("Compare strings case insensitive") {
    CHECK(compareStringCaseInsensitive(CStringIterator("ABC"),
                                       CStringIterator("ABc")) ==
          std::strong_ordering::less);
    CHECK(compareStringCaseInsensitive("ABC", "ABC") ==
          std::strong_ordering::equal);
    CHECK(compareStringCaseInsensitive("AB", "ABC") ==
          std::strong_ordering::less);
    CHECK(compareStringCaseInsensitive("a", "BC") ==
          std::strong_ordering::less);

    CHECK(CompareCaseInsensitive<>{}(std::filesystem::path("A/B"),
                                     std::filesystem::path("a/B")));
}
