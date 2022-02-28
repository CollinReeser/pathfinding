#include "Util.h"

#include <cmath>

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

double dist_euclidean(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
) {
    // Straight line along the y-axis.
    if (x1 == x2) {
        return (y1 > y2) ? y1 - y2 : y2 - y1;
    }
    // Straight line along the x-axis.
    else if (y1 == y2) {
        return (x1 > x2) ? x1 - x2 : x2 - x1;
    }
    // Straight line in an ordinal direction (NW, SE, etc), which will be
    // exactly a multiple of the sqrt of 2.
    else if (
        uint32_t axis_dist {((y1 > y2) ? y1 - y2 : y2 - y1)};
        axis_dist == ((x1 > x2) ? x1 - x2 : x2 - x1)
    ) {
        return
            1.41421356237 *
            static_cast<double>(axis_dist)
        ;
    }

    // Some arbitrary set of two points, for which we're forced to calculate
    // the Euclidean distance the long way.

    const uint32_t x_dist {
        (x1 > x2)
            ? x1 - x2
            : x2 - x1
    };

    const uint32_t y_dist {
        (y1 > y2)
            ? y1 - y2
            : y2 - y1
    };

    return
        std::sqrt(
            (x_dist * x_dist) + (y_dist * y_dist)
        )
    ;
}

double dist_chebyshev(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
) {
    return std::max(
        ((x1 > x2) ? x1 - x2 : x2 - x1),
        ((y1 > y2) ? y1 - y2 : y2 - y1)
    );
}

double dist_manhattan(
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2
) {
    return (
        ((x1 > x2) ? x1 - x2 : x2 - x1) +
        ((y1 > y2) ? y1 - y2 : y2 - y1)
    );
}