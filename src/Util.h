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
