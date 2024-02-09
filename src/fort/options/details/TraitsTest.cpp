#include "Traits.hpp"
#include <gtest/gtest.h>

#include <list>

class Formatable {};

std::ostream &operator<<(std::ostream &out, const Formatable &) {
	return out;
}

class Parsable {};

std::istream &operator>>(std::istream &in, const Parsable &) {
	return in;
}

class Both {};

std::ostream &operator<<(std::ostream &out, const Both &) {
	return out;
}

std::istream &operator>>(std::istream &in, const Both &) {
	return in;
}

namespace fort {
namespace options {
namespace details {

class TraitsTest : public ::testing::Test {};

TEST_F(TraitsTest, IntegerType) {
	EXPECT_FALSE(is_optionable_v<char>);
	EXPECT_FALSE(is_optionable_v<unsigned char>);
	EXPECT_FALSE(is_optionable_v<uint8_t>);
	EXPECT_TRUE(is_optionable_v<int8_t>);
	EXPECT_TRUE(is_optionable_v<int>);
	EXPECT_TRUE(is_optionable_v<long>);
	EXPECT_TRUE(is_optionable_v<long long>);
}

TEST_F(TraitsTest, FloatingType) {
	EXPECT_TRUE(is_optionable_v<float>);
	EXPECT_TRUE(is_optionable_v<double>);
}

TEST_F(TraitsTest, String) {
	EXPECT_TRUE(is_optionable_v<std::string>);
}

TEST_F(TraitsTest, Bool) {
	EXPECT_TRUE(is_optionable_v<bool>);
}

TEST_F(TraitsTest, Custom) {
	EXPECT_FALSE(is_optionable_v<Parsable>);
	EXPECT_FALSE(is_optionable_v<Formatable>);
	EXPECT_TRUE(is_optionable_v<Both>);
}

TEST_F(TraitsTest, Specialization) {
	EXPECT_TRUE((is_specialization_v<std::vector<int>, std::vector>));
	EXPECT_FALSE((is_specialization_v<std::vector<int>, std::list>));
	EXPECT_FALSE((is_specialization_v<int, std::list>));
}

} // namespace details
} // namespace options
} // namespace fort
