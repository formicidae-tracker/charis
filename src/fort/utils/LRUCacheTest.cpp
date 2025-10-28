#include "LRUCache.hpp"
#include <gtest/gtest.h>

namespace fort {
namespace utils {

int add_values(int a, int b) {
	return a + b;
}

static constexpr size_t N = 4;

class LRUCacheTest : public ::testing::Test {
protected:
	using Cache = LRUCache<N, std::function<int(int, int)>>;

	Cache cache = Cache{add_values};

	void SetUp() {
		cache = Cache{add_values};
	}
};

TEST_F(LRUCacheTest, CallOnlyIfNeeded) {
	EXPECT_FALSE(cache.Contains({2, 2}));

	EXPECT_EQ(cache(2, 2), 4);

	EXPECT_TRUE(cache.Contains({2, 2}));
}

TEST_F(LRUCacheTest, RemovesOldest) {

	for (int i = 0; i < 100; ++i) {
		EXPECT_EQ(cache(1, i), 1 + i);
		EXPECT_TRUE(cache.Contains({1, i}));
	}

	for (size_t i = 0; i < N; ++i) {
		EXPECT_TRUE(cache.Contains({1, 100 - i - 1}));
	}
	EXPECT_FALSE(cache.Contains({1, 99 - N}));
	EXPECT_FALSE(cache.Contains({1, 100}));
}

} // namespace utils
} // namespace fort
