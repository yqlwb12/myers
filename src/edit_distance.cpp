// SPDX-License-Identifier: MIT

#include "myers/edit_distance.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

namespace myers {
namespace {

using word = std::uint64_t;

/// Myers' bit-vector algorithm (Hyyrö's formulation), pattern <= 64 bytes.
///
/// `peq[c]` marks every position where `c` occurs in the pattern.  The pair
/// (vp, vn) is the "positive/negative vertical delta" state; the horizontal
/// deltas for one text character fall out of a single addition, which is the
/// whole trick.
std::size_t bit_parallel(std::string_view pattern, std::string_view text) noexcept {
    const std::size_t m = pattern.size();
    if (m == 0) {
        return text.size();
    }

    std::array<word, 256> peq{};
    for (std::size_t i = 0; i < m; ++i) {
        peq[static_cast<unsigned char>(pattern[i])] |= (word{1} << i);
    }

    const word last = word{1} << (m - 1);
    word vp = ~word{0};
    word vn = 0;
    std::ptrdiff_t score = static_cast<std::ptrdiff_t>(m);

    for (const char raw : text) {
        const word pm = peq[static_cast<unsigned char>(raw)];
        const word x = pm | vn;
        const word d0 = ((vp + (x & vp)) ^ vp) | x;
        const word hp = vn | ~(d0 | vp);
        const word hn = vp & d0;
        if (hp & last) {
            ++score;
        }
        if (hn & last) {
            --score;
        }
        // The shift-in bit is 1, not 0: a new column starts with a deletion
        // available from the virtual row above.  Getting this wrong produces a
        // metric that is plausible and wrong (it under-counts by exactly the
        // number of positions where the texts disagree after aligning).
        const word shifted = (hp << 1) | 1;
        vp = (hn << 1) | ~(d0 | shifted);
        vn = shifted & d0;
    }

    return static_cast<std::size_t>(score);
}

/// Two-row dynamic programme, O(n*m) time and O(m) memory.
std::size_t table_distance(std::string_view a, std::string_view b) {
    if (a.size() < b.size()) {
        std::swap(a, b);
    }
    std::vector<std::size_t> previous(b.size() + 1);
    std::vector<std::size_t> current(b.size() + 1);
    std::iota(previous.begin(), previous.end(), std::size_t{0});

    for (std::size_t i = 1; i <= a.size(); ++i) {
        current[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t substitution =
                previous[j - 1] + (a[i - 1] == b[j - 1] ? 0u : 1u);
            current[j] = std::min({previous[j] + 1, current[j - 1] + 1, substitution});
        }
        previous.swap(current);
    }
    return previous[b.size()];
}

}  // namespace

std::size_t distance(std::string_view a, std::string_view b) noexcept {
    if (a.size() > b.size()) {
        std::swap(a, b);
    }
    if (a.empty()) {
        return b.size();
    }
    if (a.size() <= kBitParallelLimit) {
        return bit_parallel(a, b);
    }
    return table_distance(a, b);
}

bool within(std::string_view a, std::string_view b, std::size_t limit) noexcept {
    if (a.size() > b.size()) {
        std::swap(a, b);
    }
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    if (m - n > limit) {
        return false;
    }

    const std::size_t outside = limit + 1;
    std::vector<std::size_t> previous(m + 1, outside);
    std::vector<std::size_t> current(m + 1, outside);

    for (std::size_t j = 0; j <= m && j <= limit; ++j) {
        previous[j] = j;
    }

    for (std::size_t i = 1; i <= n; ++i) {
        std::fill(current.begin(), current.end(), outside);
        if (i <= limit) {
            current[0] = i;
        }
        const std::size_t first = i > limit ? i - limit : 1;
        const std::size_t last = std::min(m, i + limit);
        for (std::size_t j = first; j <= last; ++j) {
            const std::size_t substitution =
                previous[j - 1] + (a[i - 1] == b[j - 1] ? 0u : 1u);
            current[j] = std::min({previous[j] + 1, current[j - 1] + 1, substitution});
        }
        previous.swap(current);
    }

    return previous[m] <= limit;
}

std::size_t lcs(std::string_view a, std::string_view b) {
    if (a.empty() || b.empty()) {
        return 0;
    }
    std::vector<std::size_t> previous(b.size() + 1, 0);
    std::vector<std::size_t> current(b.size() + 1, 0);

    for (std::size_t i = 1; i <= a.size(); ++i) {
        for (std::size_t j = 1; j <= b.size(); ++j) {
            if (a[i - 1] == b[j - 1]) {
                current[j] = previous[j - 1] + 1;
            } else {
                current[j] = std::max(previous[j], current[j - 1]);
            }
        }
        previous.swap(current);
    }
    return previous[b.size()];
}

double similarity(std::string_view a, std::string_view b) noexcept {
    const std::size_t longest = std::max(a.size(), b.size());
    if (longest == 0) {
        return 1.0;
    }
    const std::size_t distance_value = distance(a, b);
    return 1.0 - static_cast<double>(distance_value) / static_cast<double>(longest);
}

}  // namespace myers
