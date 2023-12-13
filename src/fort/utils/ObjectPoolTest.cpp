#include <gtest/gtest.h>

#include "ObjectPool.hpp"

namespace fort {
namespace utils {

class ObjectPoolTest : public ::testing::Test {

protected:
	void SetUp() override {
		d_count = 0;
	}

	void Increment() noexcept {
		d_count++;
	}

	void Decrement() noexcept {
		d_count--;
	}

	int Count() const noexcept {
		return d_count;
	}

private:
	int d_count = 0;
};

TEST_F(ObjectPoolTest, AllocateOnce) {
	struct A {};

	auto build = [this]() {
		Increment();
		return new A{};
	};
	auto pool = ObjectPool<A, decltype(build)>::Create(build);

	A *expected = nullptr;

	for (int i = 0; i < 10; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto res = pool->Get();
		EXPECT_EQ(Count(), 1);
		if (expected != nullptr) {
			EXPECT_EQ(res.get(), expected);
		}
		expected = res.get();
	}
}

TEST_F(ObjectPoolTest, DefaultBuilder) {
	struct A {};

	auto pool = ObjectPool<A>::Create();

	A *expected = nullptr;

	for (int i = 0; i < 10; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto res = pool->Get();
		if (expected != nullptr) {
			EXPECT_EQ(res.get(), expected);
		}
		expected = res.get();
	}
}

TEST_F(ObjectPoolTest, NonDefaultConstructible) {
	struct A {
		A(int) {}
	};

	auto builder = []() { return new A{1}; };
	auto pool    = ObjectPool<A, decltype(builder)>::Create(builder);

	A *expected = nullptr;

	for (int i = 0; i < 10; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto res = pool->Get();
		if (expected != nullptr) {
			EXPECT_EQ(res.get(), expected);
		}
		expected = res.get();
	}
}

TEST_F(ObjectPoolTest, DeallocateAll) {
	struct A {};

	auto build = [this]() {
		Increment();
		return new A{};
	};
	auto deleter = [this](struct A *a) {
		Decrement();
		delete a;
	};
	auto pool = ObjectPool<A, decltype(build), decltype(deleter)>::Create(
	    build,
	    deleter
	);

	A *expected = nullptr;

	for (int i = 0; i < 10; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto res = pool->Get();
		EXPECT_EQ(Count(), 1);
		if (expected != nullptr) {
			EXPECT_EQ(res.get(), expected);
		}
		expected = res.get();
	}
	EXPECT_EQ(Count(), 1);
	pool.reset(); // this will call the deleter

	EXPECT_EQ(Count(), 0);
}

} // namespace utils
} // namespace fort
