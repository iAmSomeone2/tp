//
// Created by Brenden Davidson on 10/2/26.
//

#include <nightfall/compat/globals.hpp>
#include <gtest/gtest.h>
#include <numbers>

constexpr float pi = std::numbers::pi_v<float>;

TEST(GlobalsTest, DEG_TO_RAD) {
    constexpr float result = DEG_TO_RAD(90.0);
    EXPECT_EQ(result, pi * 0.5);
}

TEST(GlobalsTest, RAD_TO_DEG) {
    constexpr float result = RAD_TO_DEG(pi * 0.5);
    EXPECT_EQ(result, 90.f);
}