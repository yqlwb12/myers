# myers

[![tests](https://github.com/yqlwb12/myers/actions/workflows/ci.yml/badge.svg)](https://github.com/yqlwb12/myers/actions/workflows/ci.yml)

> **Experimental project, for reference and learning only.**
> Published as-is, with no warranty of correctness or fitness for purpose.
> 本项目仅供参考与实验用途，不保证可靠性；生产使用前请自行充分测试。

**Edit distance with a bit-parallel core.** C++17, no dependencies, one header, and a fast path that does 64 pattern bytes per machine word.

```console
$ myers kitten sitting
distance   3
lcs        4
similarity 0.571
within(1)  no
```

## Why

The textbook Levenshtein table is O(n·m) and touches memory `n·m` times. Myers'
1999 bit-vector formulation collapses the whole column into one machine-word
addition: with a 64-byte pattern, each input byte costs a handful of integer
ops and no memory access beyond the pattern alphabet.

This library is the honest version of that: bit-parallel where it applies,
a two-row dynamic programme where it does not, and a test suite that checks the
two paths against a third, independent implementation.

## Install

```console
$ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
$ ctest --test-dir build --output-on-failure
```

Header and source, no packaging magic:

```cmake
add_subdirectory(myers)
target_link_libraries(your_target PRIVATE myers::myers)
```

## API

```cpp
#include "myers/edit_distance.hpp"

myers::distance("kitten", "sitting");        // 3
myers::within("kitten", "sitting", 2);       // false, and cheap to say
myers::lcs("AGGTAB", "GXTXAYB");             // 4
myers::similarity("kitten", "sitting");      // 0.571...
```

| Function | Cost | Notes |
| --- | --- | --- |
| `distance(a, b)` | O(n·⌈m/64⌉) when min(n,m) ≤ 64, else O(n·m) | byte-oriented |
| `within(a, b, k)` | O(k·min(n, m)) | never builds the full table |
| `lcs(a, b)` | O(n·m), O(m) memory | plain table |
| `similarity(a, b)` | same as `distance` | `1 - d/max(len)`, in [0,1] |

## How it works

The pattern becomes a set of bitmasks: `peq[c]` has a 1 at every position where
byte `c` occurs. The scan keeps two words, `VP` and `VN`, holding the positive
and negative vertical deltas of the DP frontier. For each byte of the text:

```text
X  = peq[c] | VN
D0 = ((VP + (X & VP)) ^ VP) | X      // one addition, that is the whole trick
HP = VN | ~(D0 | VP)
HN = VP & D0
score += bit(HP, m-1) - bit(HN, m-1)
VP = (HN << 1) | ~(D0 | (HP << 1))
VN = (HP << 1) & D0
```

The score starts at the pattern length and moves by one per input byte. Six
operations plus a shift, per byte, for the entire column of the table.

## Correctness

The tests do not trust the implementation. They carry a separate, obvious
dynamic programme and compare:

- every pair of strings over `{a,b}` up to length 4 — 961 pairs, exhaustively;
- 400 random pairs up to 80 bytes, which exercise the bit-parallel path;
- 40 random pairs of 70–130 bytes, which force the table fallback;
- the exact 63/64/65-byte boundary, where an off-by-one hides in the shift;
- `within()` against `distance()` for every limit from 0 to 6 on 120 pairs;
- LCS and similarity bounds, including the empty-string corners.

A bit-parallel reimplementation that is "probably right" is a liability; the
oracle is the point. It has already earned its keep: the first version of
`bit_parallel` shifted `HP` in with a zero instead of a one, which is a
plausible-looking line that under-counts by exactly one per disagreement and
passes every hand-written example. The exhaustive sweep failed 314 of 961 pairs
and said so immediately.

`3860 checks, 0 failures` is what the binary prints today.

## Known limits

- **Bytes, not characters.** `"é"` is two bytes in UTF-8 and counts twice. For
  code points, convert first.
- **The fast path stops at 64.** Longer patterns fall back to O(n·m), which is
  correct but ordinary; the block-wise Myers variant that handles arbitrary
  lengths is deliberately not here.
- **Unit costs only.** No weighted substitutions, no transposition (that is
  Damerau–Levenshtein, a different recurrence).

## Licence

MIT.
