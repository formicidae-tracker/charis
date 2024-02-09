#include "Optionable.hpp"
#include <gtest/gtest.h>

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

class OptionableTest : public ::testing::Test {};

TEST_F(OptionableTest, IntegerType) {
	EXPECT_FALSE(is_optionable_v<char>);
	EXPECT_FALSE(is_optionable_v<unsigned char>);
	EXPECT_FALSE(is_optionable_v<uint8_t>);
	EXPECT_TRUE(is_optionable_v<int8_t>);
	EXPECT_TRUE(is_optionable_v<int>);
	EXPECT_TRUE(is_optionable_v<long>);
	EXPECT_TRUE(is_optionable_v<long long>);
}

TEST_F(OptionableTest, FloatingType) {
	EXPECT_TRUE(is_optionable_v<float>);
	EXPECT_TRUE(is_optionable_v<double>);
}

TEST_F(OptionableTest, String) {
	EXPECT_TRUE(is_optionable_v<std::string>);
}

TEST_F(OptionableTest, Bool) {
	EXPECT_TRUE(is_optionable_v<bool>);
}

TEST_F(OptionableTest, Custom) {
	EXPECT_FALSE(is_optionable_v<Parsable>);
	EXPECT_FALSE(is_optionable_v<Formatable>);
	EXPECT_TRUE(is_optionable_v<Both>);
}

} // namespace details
} // namespace options
} // namespace fort
