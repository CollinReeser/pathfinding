#include "Util.h"

std::pair<uint32_t, uint32_t> get_node_xy(
    const uint32_t i, const uint32_t width
) {
    const uint32_t y = i / width;
    const uint32_t x = i - (y * width);

    return {x, y};
}
