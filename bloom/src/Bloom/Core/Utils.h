#ifndef BLOOM_CORE_UTILS_H
#define BLOOM_CORE_UTILS_H

#include <algorithm>
#include <cctype>
#include <compare>
#include <filesystem>
#include <ranges>

namespace bloom {

/// Iterator wrapper around a zero terminated array
template <typename CharT>
struct CStringIterator {
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv_t<CharT>;
    using pointer = CharT*;
    using reference = CharT&;
    using iterator_category = std::bidirectional_iterator_tag;

    CStringIterator() = default;

    explicit CStringIterator(CharT* p): p(p) {}

    CStringIterator& operator++() {
        ++p;
        return *this;
    }

    CStringIterator& operator--() {
        --p;
        return *this;
    }

    CStringIterator operator++(int) {
        auto copy = *this;
        ++*this;
        return copy;
    }

    CStringIterator operator--(int) {
        auto copy = *this;
        --*this;
        return copy;
    }

    reference operator*() const { return *p; }

    bool operator==(CStringIterator) const { return !*p; }

    CStringIterator begin() const { return *this; }

    CStringIterator end() const { return {}; }

private:
    CharT* p = nullptr;
};

/// Lexicographically compares the string `[begin1, end1)` and `[begin2, end2)`
/// where a character `a` precedes another character `b` if `a` precedes `b` in
/// the alphabet ignoring case. If `a` and `b` are equal when ignoring case,
/// then the uppercase character precedes the lowercase character.
template <typename Itr1, typename S1 = Itr1, typename Itr2, typename S2 = Itr2>
std::strong_ordering compareStringCaseInsensitive(Itr1 begin1, S1 end1,
                                                  Itr2 begin2, S2 end2) {
    auto [i, j] = std::mismatch(begin1, end1, begin2, end2);
    if (i == end1 && j == end2) {
        return std::strong_ordering::equal;
    }
    if (i == end1) {
        return std::strong_ordering::less;
    }
    if (j == end2) {
        return std::strong_ordering::greater;
    }
    auto A = std::toupper(*i);
    auto B = std::toupper(*j);
    if (A == B) {
        return *i <=> *j;
    }
    return A <=> B;
}

/// \overload
std::strong_ordering compareStringCaseInsensitive(std::ranges::range auto&& a,
                                                  std::ranges::range auto&& b) {
    return compareStringCaseInsensitive(std::ranges::begin(a),
                                        std::ranges::end(a),
                                        std::ranges::begin(b),
                                        std::ranges::end(b));
}

/// Comparison object that uses `compareStringCaseInsensitive` to be used in
/// maps and sets
template <auto Mode = [] { return std::strong_ordering::less; }>
struct CompareCaseInsensitive {
    bool operator()(std::filesystem::path const& a,
                    std::filesystem::path const& b) const {
        return compareStringCaseInsensitive(CStringIterator(a.c_str()),
                                            CStringIterator(b.c_str())) ==
               Mode();
    }
};

} // namespace bloom

#endif // BLOOM_CORE_UTILS_H
