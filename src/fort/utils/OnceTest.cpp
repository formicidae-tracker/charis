#include "Once.hpp"
#include <gtest/gtest.h>

namespace fort {
namespace utils {

class OnceTest : public ::testing::Test {

protected:
	void SetUp() override {
		d_count = 0;
	}

	void Increment() noexcept {
		++d_count;
	}

	int Count() const noexcept {
		return d_count;
	}

private:
	int d_count = 0;
};

TEST_F(OnceTest, ItActuallyOnce) {

	auto function = [this]() {
		once {
			Increment();
		};
	};

	function();
	function();
	function();
	ASSERT_EQ(Count(), 1);
}

} // namespace utils
} // namespace fort
