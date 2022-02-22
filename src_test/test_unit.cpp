
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

TEST(Util, NodeIndexSigned) {
    // Width must be >0, but no other restrictions on coordinates.
    EXPECT_DEATH(get_node_index<int32_t>(0, 0, 0), "Assertion");
    EXPECT_DEATH(get_node_index<int32_t>(0, 1, 0), "Assertion");
    EXPECT_DEATH(get_node_index<int32_t>(1, 0, 0), "Assertion");
    EXPECT_DEATH(get_node_index<int32_t>(1, 1, 0), "Assertion");

    EXPECT_EQ(get_node_index<int32_t>(0, 0, 3), 0);
    EXPECT_EQ(get_node_index<int32_t>(1, 0, 3), 1);
    EXPECT_EQ(get_node_index<int32_t>(2, 0, 3), 2);

    EXPECT_EQ(get_node_index<int32_t>(0, 1, 3), 3);
    EXPECT_EQ(get_node_index<int32_t>(1, 1, 3), 4);
    EXPECT_EQ(get_node_index<int32_t>(2, 1, 3), 5);

    EXPECT_EQ(get_node_index<int32_t>(0, 2, 3), 6);
    EXPECT_EQ(get_node_index<int32_t>(1, 2, 3), 7);
    EXPECT_EQ(get_node_index<int32_t>(2, 2, 3), 8);

    EXPECT_EQ(get_node_index<int32_t>( 0, 0, 3),  0);
    EXPECT_EQ(get_node_index<int32_t>(-0, 0, 3),  0);
    EXPECT_EQ(get_node_index<int32_t>(-1, 0, 3), -1);
    EXPECT_EQ(get_node_index<int32_t>(-2, 0, 3), -2);

    EXPECT_EQ(get_node_index<int32_t>(-0, -1, 3), -3);
    EXPECT_EQ(get_node_index<int32_t>(-1, -1, 3), -4);
    EXPECT_EQ(get_node_index<int32_t>(-2, -1, 3), -5);

    EXPECT_EQ(get_node_index<int32_t>(-0, -2, 3), -6);
    EXPECT_EQ(get_node_index<int32_t>(-1, -2, 3), -7);
    EXPECT_EQ(get_node_index<int32_t>(-2, -2, 3), -8);

    // A negative x coordinate can move the logical y coordinate "down" a level,
    // and the abs(x) of a negative x coordinate can be larger than the width.
    EXPECT_EQ(get_node_index<int32_t>(-0, 1, 3),  3);
    EXPECT_EQ(get_node_index<int32_t>(-1, 1, 3),  2);
    EXPECT_EQ(get_node_index<int32_t>(-2, 1, 3),  1);
    EXPECT_EQ(get_node_index<int32_t>(-3, 1, 3),  0);
    EXPECT_EQ(get_node_index<int32_t>(-4, 1, 3), -1);

    // An x coordinate that is larger than the width can move the logical y
    // coordinate "up" a level.
    EXPECT_EQ(get_node_index<int32_t>(0, 1, 3), 3);
    EXPECT_EQ(get_node_index<int32_t>(1, 1, 3), 4);
    EXPECT_EQ(get_node_index<int32_t>(2, 1, 3), 5);
    EXPECT_EQ(get_node_index<int32_t>(3, 1, 3), 6);
    EXPECT_EQ(get_node_index<int32_t>(4, 1, 3), 7);

    // The above rules apply essentially in "reverse" if the y coordinate is
    // negative. In other words, a positive x coordinate will always push the
    // final index further to the right along the number line, and a negative x
    // coordinate will always push the final index further to the left along the
    // number line.
    EXPECT_EQ(get_node_index<int32_t>(-0, -1, 3), -3);
    EXPECT_EQ(get_node_index<int32_t>(-1, -1, 3), -4);
    EXPECT_EQ(get_node_index<int32_t>(-2, -1, 3), -5);
    EXPECT_EQ(get_node_index<int32_t>(-3, -1, 3), -6);
    EXPECT_EQ(get_node_index<int32_t>(-4, -1, 3), -7);
    EXPECT_EQ(get_node_index<int32_t>(0, -1, 3), -3);
    EXPECT_EQ(get_node_index<int32_t>(1, -1, 3), -2);
    EXPECT_EQ(get_node_index<int32_t>(2, -1, 3), -1);
    EXPECT_EQ(get_node_index<int32_t>(3, -1, 3), 0);
    EXPECT_EQ(get_node_index<int32_t>(4, -1, 3), 1);

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
