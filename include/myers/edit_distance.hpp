// SPDX-License-Identifier: MIT
//
// myers -- byte-oriented edit distance with a bit-parallel fast path.
//
// The interesting part is the constant factor: Myers (1999) turns an O(n*m)
// dynamic programme into O(n * ceil(m/64)) integer operations, so a 64-byte
// pattern costs one machine word per input byte.  Everything here works on
// bytes, not on code points or grapheme clusters.

#ifndef MYERS_EDIT_DISTANCE_HPP
#define MYERS_EDIT_DISTANCE_HPP

#include <cstddef>
#include <string_view>

namespace myers {

/// Longest pattern the bit-parallel path can handle: one 64-bit word.
constexpr std::size_t kBitParallelLimit = 64;

/// Levenshtein distance (insert, delete, substitute -- all cost 1).
///
/// Uses the bit-parallel algorithm when the shorter side fits in a word, and
/// falls back to a two-row dynamic programme otherwise.  Both paths return the
/// same number; the tests check that on purpose.
std::size_t distance(std::string_view a, std::string_view b) noexcept;

/// True when `distance(a, b) <= limit`, without computing the distance when it
/// exceeds the limit.  Banded dynamic programme, O(limit * min(n, m)).
bool within(std::string_view a, std::string_view b, std::size_t limit) noexcept;

/// Length of the longest common subsequence.  Classic O(n*m) table; kept
/// simple because LCS has no bit-parallel trick quite as tidy as Myers'.
std::size_t lcs(std::string_view a, std::string_view b);

/// 1 - distance / max(len), in [0, 1]: 1.0 for identical strings, 0.0 when
/// they share nothing.  Two empty strings are identical, so 1.0.
double similarity(std::string_view a, std::string_view b) noexcept;

}  // namespace myers

#endif  // MYERS_EDIT_DISTANCE_HPP
