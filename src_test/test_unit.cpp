
#include <iostream>

#include "Util.h"

#include "gtest/gtest.h"

TEST(Util, NodeIndex) {
    EXPECT_DEATH(get_node_index(0, 0, 0), "Assertion");
    EXPECT_DEATH(get_node_index(0, 1, 0), "Assertion");
    EXPECT_DEATH(get_node_index(1, 0, 0), "Assertion");
    EXPECT_DEATH(get_node_index(1, 1, 0), "Assertion");

    EXPECT_DEATH(get_node_index(3, 0, 3), "Assertion");
    EXPECT_DEATH(get_node_index(3, 1, 3), "Assertion");
    EXPECT_DEATH(get_node_index(4, 0, 3), "Assertion");
    EXPECT_DEATH(get_node_index(4, 1, 3), "Assertion");

    EXPECT_EQ(get_node_index(0, 0, 3), 0u);
    EXPECT_EQ(get_node_index(1, 0, 3), 1u);
    EXPECT_EQ(get_node_index(2, 0, 3), 2u);

    EXPECT_EQ(get_node_index(0, 1, 3), 3u);
    EXPECT_EQ(get_node_index(1, 1, 3), 4u);
    EXPECT_EQ(get_node_index(2, 1, 3), 5u);

    EXPECT_EQ(get_node_index(0, 2, 3), 6u);
    EXPECT_EQ(get_node_index(1, 2, 3), 7u);
    EXPECT_EQ(get_node_index(2, 2, 3), 8u);

    EXPECT_EQ(get_node_index(0, 3, 3), 9u);
    EXPECT_EQ(get_node_index(1, 3, 3), 10u);
    EXPECT_EQ(get_node_index(2, 3, 3), 11u);

    EXPECT_EQ(get_node_index(0, 4, 3), 12u);
    EXPECT_EQ(get_node_index(1, 4, 3), 13u);
    EXPECT_EQ(get_node_index(2, 4, 3), 14u);
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
