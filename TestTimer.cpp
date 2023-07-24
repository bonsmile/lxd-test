#include "lxd/timer.h"
#include <gtest/gtest.h>

TEST(Timer, Strptime) {
	tm result = {};
	char* pStr = lxd::strptime("2023-07-30", "%Y-%m-%d", &result);
	EXPECT_EQ(result.tm_year, 2023 - 1900);
	EXPECT_EQ(result.tm_mon, 7 - 1);
	EXPECT_EQ(result.tm_mday, 30);
}