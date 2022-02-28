#include "Util.h"

uint32_t get_node_index(
    const uint32_t x, const uint32_t y, const uint32_t width
) {
    assert(width > 0);
    assert(x < width);

    return y * width + x;
}

std::pair<uint32_t, uint32_t> get_node_xy(
    const uint32_t i, const uint32_t width
) {
    const uint32_t y = i / width;
    const uint32_t x = i - (y * width);

    return {x, y};
}
