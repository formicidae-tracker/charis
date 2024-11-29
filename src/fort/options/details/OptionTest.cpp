#include <gtest/gtest.h>

#include "Option.hpp"
#include <chrono>
#include <fort/utils/Defer.hpp>
#include <iomanip>
#include <ios>
#include <ratio>
#include <regex>
#include <sstream>
#include <string>

namespace fort {
namespace options {
namespace details {

class OptionTest : public testing::Test {};

TEST_F(OptionTest, BoolHaveEnforcedDefaultValueAndRequirement) {
	auto opt = Option<bool>{
	    {
	        .ShortFlag   = 0,
	        .Name        = "my-flag",
	        .Description = "turn my flag on",
	        .Required    = true,
	    },
	};
	EXPECT_FALSE(opt.Value);
	EXPECT_FALSE(opt.Required);
	EXPECT_FALSE(opt.Repeatable);
	EXPECT_EQ(opt.NumArgs, 0);
}

TEST_F(OptionTest, BoolParsing) {
	auto opt = Option<bool>{{
	    .ShortFlag   = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	    .Required    = true,
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
	    .ShortFlag   = 0,
	    .Name        = "my-flag",
	    .Description = "turn my flag on",
	    .Required    = true,
	}};

	std::ostringstream out;
	out << std::noboolalpha;
	opt.Format(out);

	EXPECT_EQ(out.str(), "false");
	EXPECT_EQ(out.flags() & std::ios_base::boolalpha, 0);
}

TEST_F(OptionTest, IntHaveDefaultArguments) {
	auto opt = Option<int>{{
	    .ShortFlag   = 0,
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
	    .ShortFlag   = 0,
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
	     .ShortFlag   = 0,
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
	     .Required    = true,
    }};
	opt.Value = 23;
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "23");
}

TEST_F(OptionTest, FloatHaveDefaultArguments) {
	auto opt = Option<float>{{
	    .ShortFlag   = 0,
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
	    .ShortFlag   = 0,
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
	     .ShortFlag   = 0,
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
	     .Required    = true,
    }};
	opt.Value = 23;
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "23");
}

TEST_F(OptionTest, StringHaveDefaultArguments) {
	auto opt = Option<std::string>{{
	    .ShortFlag   = 0,
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
	    .ShortFlag   = 0,
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
	     .ShortFlag   = 0,
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
	     .Required    = true,
    }};
	opt.Value = "The quick brown fox jumps over the lazy dog.";
	std::ostringstream out;
	opt.Format(out);
	EXPECT_EQ(out.str(), "The quick brown fox jumps over the lazy dog.");
}

using Duration = std::chrono::duration<int64_t, std::nano>;

constexpr int64_t SECOND = 1000000000L;

std::ostream &operator<<(std::ostream &out, const Duration &d) {
	auto flags = out.flags();
	defer {
		out.flags(flags);
	};
	auto abs = std::abs(d.count());
	if (abs < 1000L) {
		return out << d.count() << "ns";
	}
	if (abs < 1000000L) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1000.0)
		           << "us";
	}

	if (abs < SECOND) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1.0e6)
		           << "ms";
	}

	if (abs < 60U * SECOND) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1.0e9)
		           << "s";
	}

	Duration seconds = Duration{d.count() % (60L * SECOND)};
	int64_t  minutes = (d.count() - seconds.count()) / 60L * SECOND;
	if (std::abs(minutes) < 60L) {
		return out << minutes << "m" << seconds;
	}

	int64_t hours = minutes / 60L;
	minutes -= 60L * hours;
	return out << hours << "h" << minutes << "m" << seconds;
}

std::istream &operator>>(std::istream &in, Duration &value) {
	static std::regex durationRx{
	    "(-)?([0-9]+h)?([0-9]+m)?([0-9\\.eE+-]+)(ns|us|ms|s)",
	};
	std::string tmp;
	in >> tmp;
	std::smatch m;
	if (std::regex_match(tmp, m, durationRx) == false || m.size() != 6) {
		in.setstate(std::ios_base::failbit);
		return in;
	}

	int64_t sign = m[1] == "-" ? -1 : 1;
	int64_t hours =
	    m[2] == "" ? 0
	               : std::stoll(std::string(m[2]).substr(0, m[2].length() - 1));
	int64_t minutes =
	    m[3] == "" ? 0
	               : std::stoll(std::string(m[3]).substr(0, m[3].length() - 1));

	std::string units = m[5];

	if ((hours != 0 || minutes != 0) && units != "s") {
		in.setstate(std::ios_base::failbit);
		return in;
	}

	double rest = std::stod(m[4]);

	double unitF = 0.0;
	if (units == "s") {
		unitF = 1e9;
	} else if (units == "ms") {
		unitF = 1e6;
	} else if (units == "us") {
		unitF = 1e3;
	} else if (units == "ns") {
		unitF = 1.0;
	} else {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	value = Duration{
	    sign * (hours * 3600L * SECOND + minutes * 60L * SECOND +
	            int64_t(rest * unitF))};

	return in;
}

TEST_F(OptionTest, CustomHaveDefaultValues) {
	auto opt = Option<Duration>{{
	    .ShortFlag   = 0,
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
	    .ShortFlag   = 0,
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
	auto opt  = Option<Duration>{{
	     .ShortFlag   = 0,
	     .Name        = "my-flag",
	     .Description = "turn my flag on",
    }};
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
