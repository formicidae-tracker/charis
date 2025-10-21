#include <gtest/gtest.h>

#include "DurationTest.hpp"

#include "Option.hpp"
#include <chrono>

#include <fort/utils/Defer.hpp>

#include <ios>
#include <sstream>
#include <string>

namespace fort {
namespace options {
namespace details {

class OptionTest : public testing::Test {};

TEST_F(OptionTest, BoolHaveEnforcedDefaultValueAndRequirement) {
	auto opt = Option<bool>{
	    {
	        .Name        = "my-flag",
	        .Description = "turn my flag on",
	    },
	};
	EXPECT_FALSE(opt.Value);
	EXPECT_FALSE(opt.Required);
	EXPECT_FALSE(opt.Repeatable);
	EXPECT_EQ(opt.NumArgs, 0);
}

TEST_F(OptionTest, BoolParsing) {
	auto opt = Option<bool>{{
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	opt.Parse(std::nullopt);
	EXPECT_TRUE(opt.Value);

	opt.Parse("false");
	EXPECT_FALSE(opt.Value);

	opt.Parse("true");
	EXPECT_TRUE(opt.Value);

	try {
		opt.Parse("foo");
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag='foo'");
	}
}

TEST_F(OptionTest, BoolFormatting) {
	auto opt = Option<bool>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};

	std::ostringstream out;
	out << std::noboolalpha;
	opt.Format(out);

	EXPECT_EQ(out.str(), "false");
	EXPECT_EQ(out.flags() & std::ios_base::boolalpha, 0);
}

TEST_F(OptionTest, IntHaveDefaultArguments) {
	auto opt = Option<int>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	EXPECT_EQ(opt.Value, 0);
	ASSERT_EQ(opt.NumArgs, 1);
	EXPECT_TRUE(opt.Required);
	opt.SetDefault(0);
	EXPECT_FALSE(opt.Required);
}

TEST_F(OptionTest, IntParsing) {
	auto opt = Option<int>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};

	opt.Parse("42");
	EXPECT_EQ(opt.Value, 42);

	try {
		opt.Parse("foo");
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag='foo'");
	}

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
}

TEST_F(OptionTest, IntFormatting) {
	auto opt  = Option<int>{{
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
    }};
	opt.Value = 23;
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "23");
}

TEST_F(OptionTest, FloatHaveDefaultArguments) {
	auto opt = Option<float>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	EXPECT_EQ(opt.Value, 0.0);
	ASSERT_EQ(opt.NumArgs, 1);
	EXPECT_TRUE(opt.Required);
	opt.SetDefault(0.0);
	EXPECT_FALSE(opt.Required);
}

TEST_F(OptionTest, FloatParsing) {
	auto opt = Option<float>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	ASSERT_EQ(opt.NumArgs, 1);

	opt.Parse("42");
	EXPECT_FLOAT_EQ(opt.Value, 42.0);

	opt.Parse("-1e-4");
	EXPECT_FLOAT_EQ(opt.Value, -1e-4);

	try {
		opt.Parse("foo");
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag='foo'");
	}

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
}

TEST_F(OptionTest, FloatFormatting) {
	auto opt  = Option<float>{{
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
    }};
	opt.Value = 23;
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "23");
}

TEST_F(OptionTest, StringHaveDefaultArguments) {
	auto opt = Option<std::string>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	EXPECT_EQ(opt.Value, "");
	ASSERT_EQ(opt.NumArgs, 1);
	EXPECT_TRUE(opt.Required);
	opt.SetDefault("");
	EXPECT_FALSE(opt.Required);
}

TEST_F(OptionTest, StringParsing) {
	auto opt = Option<std::string>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};

	opt.Parse("the lazy dog jumps over the brown fox.");
	EXPECT_EQ(opt.Value, "the lazy dog jumps over the brown fox.");

	opt.Parse("");
	EXPECT_EQ(opt.Value, "");

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
}

TEST_F(OptionTest, StringFormatting) {

	auto opt  = Option<std::string>{{
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
    }};
	opt.Value = "The quick brown fox jumps over the lazy dog.";
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "The quick brown fox jumps over the lazy dog.");
}

TEST_F(OptionTest, CustomHaveDefaultValues) {
	auto opt = Option<Duration>{{
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	// We do not expect a value for custom type. should be handled by the class
	// itself.
	// EXPECT_EQ(opt.value, std::chrono::hours{0});

	ASSERT_EQ(opt.NumArgs, 1);
	EXPECT_TRUE(opt.Required);
	opt.SetDefault(Duration(12340));
	EXPECT_FALSE(opt.Required);
}

TEST_F(OptionTest, CustomParsing) {
	auto opt = Option<Duration>{{
	    .Flag        = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	}};
	opt.Parse("23.3ms");
	EXPECT_EQ(opt.Value, Duration{int64_t(233e5)});

	opt.Parse("4us");
	EXPECT_EQ(opt.Value, Duration{int64_t(4000)});

	opt.Parse("1h0m0s");
	EXPECT_EQ(
	    opt.Value,
	    std::chrono::duration_cast<Duration>(std::chrono::hours{1})
	);

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
}

TEST_F(OptionTest, CustomFomrating) {
	auto opt  = Option<Duration>({
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
    });
	opt.Value = Duration(12340);
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "12.340us");
}

TEST_F(OptionTest, RepeatableParsing) {

	auto opt = RepeatableOption<Duration>{{
	    .Name = "my-flag",
	}};

	EXPECT_TRUE(opt.value.empty());

	opt.SetDefault({std::chrono::hours{2}});

	opt.Parse("12us");
	opt.Parse("40ms");
	opt.Parse("2s");
	ASSERT_EQ(opt.value.size(), 4);
	EXPECT_EQ(
	    opt.value[0],
	    std::chrono::duration_cast<Duration>(std::chrono::hours{2})
	);

	EXPECT_EQ(
	    opt.value[1],
	    std::chrono::duration_cast<Duration>(std::chrono::microseconds{12})
	);
	EXPECT_EQ(
	    opt.value[2],
	    std::chrono::duration_cast<Duration>(std::chrono::milliseconds{40})
	);
	EXPECT_EQ(
	    opt.value[3],
	    std::chrono::duration_cast<Duration>(std::chrono::seconds{2})
	);
}

TEST_F(OptionTest, RepeatableFormatting) {
	auto opt  = RepeatableOption<Duration>{{
	     .Name = "my-flag",
    }};
	opt.value = {
	    std::chrono::microseconds{12},
	    std::chrono::milliseconds{40},
	    std::chrono::seconds{2},
	};

	{
		std::ostringstream out;
		opt.Format(out);
		EXPECT_EQ(out.str(), "[12.000us, 40.000ms, 2.000s]");
	}
	opt.value.clear();

	{
		std::ostringstream out;
		opt.Format(out);
		EXPECT_EQ(out.str(), "[]");
	}
}

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM

enum class MyEnum {
	NONE = 0,
	SOME = 1,
	ALL  = 2,
};

TEST_F(OptionTest, EnumDefaultValue) {
	auto opt = Option<MyEnum>{
	    {
	        .Name = "my-flag",
	    },
	};
	// set to the first enum
	EXPECT_EQ(opt.Value, MyEnum::NONE);
	EXPECT_TRUE(opt.Required);
	EXPECT_FALSE(opt.Repeatable);
}

TEST_F(OptionTest, EnumParsing) {
	auto opt = Option<MyEnum>{
	    {
	        .Name = "my-flag",
	    },
	};
	// set to the first enum
	EXPECT_NO_THROW(opt.Parse("SOME"));
	EXPECT_EQ(opt.Value, MyEnum::SOME);

	try {
		opt.Parse("foo");
		ADD_FAILURE() << "Should have thrown an exception, it doesn't";
	} catch (const ParseEnumError<MyEnum> &e) {
		EXPECT_STREQ(
		    e.what(),
		    "could not parse my-flag='foo': possible enum values are ['NONE', "
		    "'SOME', 'ALL']"
		);
	}
}

TEST_F(OptionTest, EnumFormating) {
	auto opt = Option<MyEnum>{
	    {
	        .Name = "my-flag",
	    },
	};

	{
		std::ostringstream out;
		opt.Format(out);
		EXPECT_EQ(out.str(), "NONE");
	}
	opt.Value = MyEnum::ALL;
	{
		std::ostringstream out;
		opt.Format(out);
		EXPECT_EQ(out.str(), "ALL");
	}
}

#endif
} // namespace details
} // namespace options
} // namespace fort
