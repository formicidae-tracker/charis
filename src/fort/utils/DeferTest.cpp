#include "Defer.hpp"

#include <gtest/gtest.h>

namespace fort {
namespace apollo {
class DeferTest : public ::testing::Test {};

TEST_F(DeferTest, ItActuallyDefers) {

	int value = 0;
	{
		value = 1;
		defer {
			value = 0;
		};

		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 0);
}

TEST_F(DeferTest, ItCanDeferMultipleTime) {

	int value = 0;
	{
		value = 1;
		defer {
			value = 3;
		};
		defer {
			value = 2;
		};

		EXPECT_EQ(value, 1);
	}
	EXPECT_EQ(value, 3);
}

} // namespace apollo
} // namespace fort
