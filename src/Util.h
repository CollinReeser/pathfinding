#ifndef UTIL_H
#define UTIL_H

#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

#include <assert.h>

uint32_t get_node_index(
    const uint32_t x, const uint32_t y, const uint32_t width
);

std::pair<uint32_t, uint32_t> get_node_xy(
    const uint32_t i, const uint32_t width
);

double dist_euclidean(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
);

double dist_chebyshev(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
);

double dist_manhattan(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
);

class Guard {
private:
    std::function<void()> fn_guard;

public:
    Guard(std::function<void()> fn_guard):
        fn_guard(fn_guard)
    {}

    ~Guard() {
        fn_guard();
    }

    void dismiss() {
        fn_guard = [](){};
    }
};

#endif
