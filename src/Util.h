#ifndef UTIL_H
#define UTIL_H

#include <concepts>
#include <cstdint>
#include <utility>

#include <assert.h>

template <typename T = uint32_t>
    requires std::integral<T>
T get_node_index(
    const T x, const T y, const uint32_t width
) {
    assert(width > 0);

    if constexpr (std::unsigned_integral<T>) {
        assert(x < width);

        return y * width + x;
    }

    else if constexpr (std::signed_integral<T>) {
        return y * width + x;
    }
}

std::pair<uint32_t, uint32_t> get_node_xy(
    const uint32_t i, const uint32_t width
);

#endif
