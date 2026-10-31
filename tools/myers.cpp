// SPDX-License-Identifier: MIT
//
// myers <a> <b> -- a one-line sanity check from a shell.

#include <cstdio>
#include <string_view>

#include "myers/edit_distance.hpp"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <a> <b>\n", argv[0]);
        return 2;
    }
    const std::string_view a{argv[1]};
    const std::string_view b{argv[2]};

    std::printf("distance   %zu\n", myers::distance(a, b));
    std::printf("lcs        %zu\n", myers::lcs(a, b));
    std::printf("similarity %.3f\n", myers::similarity(a, b));
    std::printf("within(1)  %s\n", myers::within(a, b, 1) ? "yes" : "no");
    return 0;
}
