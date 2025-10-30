#include "LRUCache.hpp"
#include <cstdlib>
#include <gtest/gtest.h>
#include <type_traits>

namespace fort {
namespace utils {

int add_values(const int &a, const int &b) {
	return a + b;
}

static constexpr size_t N = 4;

class LRUCacheTest : public ::testing::Test {
protected:
	using Cache = LRUCache<N, int (*)(const int &, const int &)>;
	static_assert(
	    std::is_same_v<Cache::KeyType, std::tuple<int, int>>,
	    "Types are not decayed"
	);
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

TEST_F(LRUCacheTest, LLIntegrity) {
	for (size_t i = 0; i < 100 * N; ++i) {
		int v = std::rand() % (4 * N);
		EXPECT_EQ(cache(1, v), 1 + v);
		EXPECT_TRUE(cache.Contains({1, v}));
	}
	// Puts the cache in a know state;
	for (int i = 1; i <= int(N); ++i) {
		EXPECT_EQ(cache(1, i), 1 + i);
		EXPECT_TRUE(cache.Contains({1, i}));
	}

	size_t current      = cache.d_oldest;
	size_t expectedNext = N;
	for (int i = 1; i <= int(N); ++i) {
		if (current >= N) {
			ADD_FAILURE() << "Failed for i=" << i << " current is out of bound";
			break;
		}
		const auto &n = cache.d_nodes[current];
		EXPECT_EQ(n.key, std::make_tuple(1, i));
		EXPECT_EQ(n.next, expectedNext);
		expectedNext = current;
		current      = n.previous;
	}
	EXPECT_EQ(current, N);
}

} // namespace utils
} // namespace fort
