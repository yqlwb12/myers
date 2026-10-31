// SPDX-License-Identifier: MIT
//
// A hand-rolled harness, on purpose: the library has one dependency and this
// test binary should not add a second.  Every check goes through CHECK/CHECK_EQ,
// failures are counted, and main() returns non-zero so ctest notices.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "myers/edit_distance.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

}  // namespace

#define CHECK(condition)                                                          \
    do {                                                                          \
        ++::g_checks;                                                             \
        if (!(condition)) {                                                       \
            ++::g_failures;                                                       \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);    \
        }                                                                         \
    } while (false)

#define CHECK_EQ(actual, expected)                                                \
    do {                                                                          \
        ++::g_checks;                                                             \
        const auto lhs_ = (actual);                                               \
        const auto rhs_ = (expected);                                             \
        if (!(lhs_ == rhs_)) {                                                    \
            ++::g_failures;                                                       \
            std::printf("  FAIL %s:%d  %s == %s  (got %lld, wanted %lld)\n",      \
                        __FILE__, __LINE__, #actual, #expected,                   \
                        static_cast<long long>(lhs_), static_cast<long long>(rhs_)); \
        }                                                                         \
    } while (false)

namespace {

/// The obvious O(n*m) dynamic programme, written here so the tests have an
/// oracle that shares no code with the implementation.
std::size_t reference_distance(std::string_view a, std::string_view b) {
    std::vector<std::size_t> previous(b.size() + 1);
    std::vector<std::size_t> current(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) {
        previous[j] = j;
    }
    for (std::size_t i = 1; i <= a.size(); ++i) {
        current[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t substitution =
                previous[j - 1] + (a[i - 1] == b[j - 1] ? 0u : 1u);
            current[j] = std::min({previous[j] + 1, current[j - 1] + 1, substitution});
        }
        std::swap(previous, current);
    }
    return previous[b.size()];
}

std::string random_string(std::mt19937& rng, std::size_t length, int alphabet) {
    std::string value;
    value.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        value.push_back(static_cast<char>('a' + static_cast<int>(rng() % static_cast<unsigned>(alphabet))));
    }
    return value;
}

void test_known_values() {
    std::puts("known values");
    CHECK_EQ(myers::distance("", ""), 0u);
    CHECK_EQ(myers::distance("", "abc"), 3u);
    CHECK_EQ(myers::distance("abc", ""), 3u);
    CHECK_EQ(myers::distance("abc", "abc"), 0u);
    CHECK_EQ(myers::distance("abc", "abd"), 1u);
    CHECK_EQ(myers::distance("kitten", "sitting"), 3u);
    CHECK_EQ(myers::distance("flaw", "lawn"), 2u);
    CHECK_EQ(myers::distance("intention", "execution"), 5u);
    CHECK_EQ(myers::similarity("abc", "abc"), 1.0);
    CHECK_EQ(myers::similarity("", ""), 1.0);
    CHECK(myers::similarity("abc", "xyz") < 0.01);
    CHECK(myers::similarity("kitten", "sitting") > 0.5);
    CHECK(myers::similarity("kitten", "sitting") < 0.7);
}

void test_symmetry() {
    std::puts("symmetry");
    std::mt19937 rng(20261031u);
    for (int round = 0; round < 200; ++round) {
        const std::string a = random_string(rng, rng() % 90, 5);
        const std::string b = random_string(rng, rng() % 90, 5);
        CHECK_EQ(myers::distance(a, b), myers::distance(b, a));
    }
}

void test_exhaustive_small() {
    std::puts("exhaustive over {a,b} up to length 4");
    std::vector<std::string> words{""};
    for (int length = 1; length <= 4; ++length) {
        const std::size_t first = words.size();
        for (int value = 0; value < (1 << length); ++value) {
            std::string word;
            for (int bit = 0; bit < length; ++bit) {
                word.push_back(((value >> bit) & 1) ? 'b' : 'a');
            }
            words.push_back(word);
        }
        CHECK(words.size() > first);
    }
    for (const std::string& a : words) {
        for (const std::string& b : words) {
            CHECK_EQ(myers::distance(a, b), reference_distance(a, b));
        }
    }
}

void test_against_reference() {
    std::puts("random pairs against the reference");
    std::mt19937 rng(42u);
    for (int round = 0; round < 400; ++round) {
        const std::string a = random_string(rng, rng() % 80, 4);
        const std::string b = random_string(rng, rng() % 80, 4);
        const std::size_t expected = reference_distance(a, b);
        CHECK_EQ(myers::distance(a, b), expected);
        CHECK_EQ(myers::within(a, b, expected), true);
        if (expected > 0) {
            CHECK_EQ(myers::within(a, b, expected - 1), false);
        }
    }
}

void test_long_inputs_use_the_table() {
    std::puts("long inputs (both sides over one word)");
    std::mt19937 rng(7u);
    for (int round = 0; round < 40; ++round) {
        const std::size_t length = 70 + (rng() % 60);
        const std::string a = random_string(rng, length, 6);
        const std::string b = random_string(rng, length, 6);
        CHECK_EQ(myers::distance(a, b), reference_distance(a, b));
    }
    // And one where the sides differ a lot in length.
    const std::string short_one = random_string(rng, 12, 6);
    const std::string long_one = random_string(rng, 140, 6);
    CHECK_EQ(myers::distance(short_one, long_one), reference_distance(short_one, long_one));
    CHECK_EQ(myers::distance(long_one, short_one), reference_distance(long_one, short_one));
}

void test_word_boundary() {
    std::puts("the 64-byte boundary");
    std::mt19937 rng(99u);
    for (const std::size_t length : {std::size_t{63}, std::size_t{64}, std::size_t{65}}) {
        const std::string a = random_string(rng, length, 3);
        const std::string b = random_string(rng, length + 3, 3);
        CHECK_EQ(myers::distance(a, b), reference_distance(a, b));
        CHECK_EQ(myers::distance(b, a), reference_distance(b, a));
    }
    CHECK_EQ(myers::kBitParallelLimit, 64u);
}

void test_within() {
    std::puts("within() agrees with distance()");
    std::mt19937 rng(1234u);
    for (int round = 0; round < 120; ++round) {
        const std::string a = random_string(rng, rng() % 40, 4);
        const std::string b = random_string(rng, rng() % 40, 4);
        const std::size_t exact = myers::distance(a, b);
        for (std::size_t limit = 0; limit <= 6; ++limit) {
            CHECK_EQ(myers::within(a, b, limit), exact <= limit);
        }
    }
    CHECK(myers::within("same", "same", 0));
    CHECK(!myers::within("a", "b", 0));
    CHECK(myers::within("abc", "abd", 1));
    CHECK(myers::within("", "abc", 3));
    CHECK(!myers::within("", "abc", 2));
    CHECK(myers::within("abc", "abcxyz", 3));
    CHECK(!myers::within("abc", "abcxyz", 2));
}

void test_lcs() {
    std::puts("longest common subsequence");
    CHECK_EQ(myers::lcs("", ""), 0u);
    CHECK_EQ(myers::lcs("abc", ""), 0u);
    CHECK_EQ(myers::lcs("AGGTAB", "GXTXAYB"), 4u);
    CHECK_EQ(myers::lcs("abcd", "abcd"), 4u);
    CHECK_EQ(myers::lcs("abc", "xyz"), 0u);
    CHECK_EQ(myers::lcs("abcdef", "ace"), 3u);

    std::mt19937 rng(555u);
    for (int round = 0; round < 60; ++round) {
        const std::string a = random_string(rng, rng() % 40, 3);
        const std::string b = random_string(rng, rng() % 40, 3);
        const std::size_t common = myers::lcs(a, b);
        CHECK(common <= std::min(a.size(), b.size()));
        CHECK(common <= std::max(a.size(), b.size()));
        // A shared subsequence is never longer than the edit distance allows.
        CHECK(myers::distance(a, b) <= a.size() + b.size() - 2 * common);
    }
}

void test_similarity_bounds() {
    std::puts("similarity bounds");
    std::mt19937 rng(31337u);
    for (int round = 0; round < 200; ++round) {
        const std::string a = random_string(rng, rng() % 50, 5);
        const std::string b = random_string(rng, rng() % 50, 5);
        const double value = myers::similarity(a, b);
        CHECK(value >= 0.0);
        CHECK(value <= 1.0);
        if (a == b) {
            CHECK(value == 1.0);
        }
    }
}

}  // namespace

int main() {
    std::puts("myers::edit_distance");

    test_known_values();
    test_symmetry();
    test_exhaustive_small();
    test_against_reference();
    test_long_inputs_use_the_table();
    test_word_boundary();
    test_within();
    test_lcs();
    test_similarity_bounds();

    std::printf("\n%d checks, %d failure%s\n", ::g_checks, ::g_failures,
                ::g_failures == 1 ? "" : "s");
    return ::g_failures == 0 ? 0 : 1;
}
