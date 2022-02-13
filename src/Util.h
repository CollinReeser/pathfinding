#ifndef UTIL_H
#define UTIL_H

#include <cstdint>
#include <utility>

uint32_t get_node_index(
    const uint32_t x, const uint32_t y, const uint32_t width
);

std::pair<uint32_t, uint32_t> get_node_xy(
    const uint32_t i, const uint32_t width
);

#endif
