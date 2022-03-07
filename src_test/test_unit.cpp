
#include <iostream>

#include "Util.h"

#include "gtest/gtest.h"

TEST(Util, NodeIndex) {
    // Width must be >0, and x-coordinate must fit in width.
    EXPECT_DEATH(get_node_index(0u, 0u, 0), "Assertion");
    EXPECT_DEATH(get_node_index(0u, 1u, 0), "Assertion");
    EXPECT_DEATH(get_node_index(1u, 0u, 0), "Assertion");
    EXPECT_DEATH(get_node_index(1u, 1u, 0), "Assertion");

    EXPECT_DEATH(get_node_index(3u, 0u, 3), "Assertion");
    EXPECT_DEATH(get_node_index(3u, 1u, 3), "Assertion");
    EXPECT_DEATH(get_node_index(4u, 0u, 3), "Assertion");
    EXPECT_DEATH(get_node_index(4u, 1u, 3), "Assertion");

    EXPECT_EQ(get_node_index(0u, 0u, 3), 0u);
    EXPECT_EQ(get_node_index(1u, 0u, 3), 1u);
    EXPECT_EQ(get_node_index(2u, 0u, 3), 2u);

    EXPECT_EQ(get_node_index(0u, 1u, 3), 3u);
    EXPECT_EQ(get_node_index(1u, 1u, 3), 4u);
    EXPECT_EQ(get_node_index(2u, 1u, 3), 5u);

    EXPECT_EQ(get_node_index(0u, 2u, 3), 6u);
    EXPECT_EQ(get_node_index(1u, 2u, 3), 7u);
    EXPECT_EQ(get_node_index(2u, 2u, 3), 8u);

    // The width does not restrict the height; it is still possible to get
    // out-of-bounds indices from this function.

    EXPECT_EQ(get_node_index(0u, 3u, 3), 9u);
    EXPECT_EQ(get_node_index(1u, 3u, 3), 10u);
    EXPECT_EQ(get_node_index(2u, 3u, 3), 11u);

    EXPECT_EQ(get_node_index(0u, 4u, 3), 12u);
    EXPECT_EQ(get_node_index(1u, 4u, 3), 13u);
    EXPECT_EQ(get_node_index(2u, 4u, 3), 14u);
}

TEST(Util, NodeXY) {
    EXPECT_EQ(get_node_xy(0, 3), std::make_pair(0u, 0u));
    EXPECT_EQ(get_node_xy(1, 3), std::make_pair(1u, 0u));
    EXPECT_EQ(get_node_xy(2, 3), std::make_pair(2u, 0u));

    EXPECT_EQ(get_node_xy(3, 3), std::make_pair(0u, 1u));
    EXPECT_EQ(get_node_xy(4, 3), std::make_pair(1u, 1u));
    EXPECT_EQ(get_node_xy(5, 3), std::make_pair(2u, 1u));

    EXPECT_EQ(get_node_xy(6, 3), std::make_pair(0u, 2u));
    EXPECT_EQ(get_node_xy(7, 3), std::make_pair(1u, 2u));
    EXPECT_EQ(get_node_xy(8, 3), std::make_pair(2u, 2u));

    EXPECT_EQ(get_node_xy(9, 3), std::make_pair(0u, 3u));
    EXPECT_EQ(get_node_xy(10, 3), std::make_pair(1u, 3u));
    EXPECT_EQ(get_node_xy(11, 3), std::make_pair(2u, 3u));

    EXPECT_EQ(get_node_xy(12, 3), std::make_pair(0u, 4u));
    EXPECT_EQ(get_node_xy(13, 3), std::make_pair(1u, 4u));
    EXPECT_EQ(get_node_xy(14, 3), std::make_pair(2u, 4u));
}

TEST(Util, Dist) {
    for (uint32_t x_start {0}; x_start < 100; ++x_start) {
        for (uint32_t y_start {0}; y_start < 100; ++y_start) {
            for (uint32_t y_end {0}; y_end < 100; ++y_end) {
                for (uint32_t x_end {0}; x_end < 100; ++x_end) {
                    if (x_start == x_end && y_start == y_end) {
                        EXPECT_EQ(
                            dist_chebyshev(x_start, y_start, x_end, y_end),
                            0
                        );
                        EXPECT_EQ(
                            dist_euclidean(x_start, y_start, x_end, y_end),
                            0
                        );
                        EXPECT_EQ(
                            dist_manhattan(x_start, y_start, x_end, y_end),
                            0
                        );
                    }
                    else if (x_start == x_end) {
                        EXPECT_EQ(
                            dist_chebyshev(x_start, y_start, x_end, y_end),
                            dist_euclidean(x_start, y_start, x_end, y_end)
                        );
                        EXPECT_EQ(
                            dist_chebyshev(x_start, y_start, x_end, y_end),
                            dist_manhattan(x_start, y_start, x_end, y_end)
                        );
                        EXPECT_EQ(
                            dist_euclidean(x_start, y_start, x_end, y_end),
                            dist_manhattan(x_start, y_start, x_end, y_end)
                        );
                    }
                    else if (y_start == y_end) {
                        EXPECT_EQ(
                            dist_chebyshev(x_start, y_start, x_end, y_end),
                            dist_euclidean(x_start, y_start, x_end, y_end)
                        );
                        EXPECT_EQ(
                            dist_chebyshev(x_start, y_start, x_end, y_end),
                            dist_manhattan(x_start, y_start, x_end, y_end)
                        );
                        EXPECT_EQ(
                            dist_euclidean(x_start, y_start, x_end, y_end),
                            dist_manhattan(x_start, y_start, x_end, y_end)
                        );
                    }

                    EXPECT_LE(
                        dist_chebyshev(x_start, y_start, x_end, y_end),
                        dist_euclidean(x_start, y_start, x_end, y_end)
                    );

                    EXPECT_LE(
                        dist_chebyshev(x_start, y_start, x_end, y_end),
                        dist_manhattan(x_start, y_start, x_end, y_end)
                    );

                    EXPECT_LE(
                        dist_euclidean(x_start, y_start, x_end, y_end),
                        dist_manhattan(x_start, y_start, x_end, y_end)
                    );
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
