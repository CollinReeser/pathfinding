
#include <iostream>

#include "Util.h"

#include "gtest/gtest.h"

TEST(Util, NodeIndexUnsigned) {
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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
