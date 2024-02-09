#include <gtest/gtest.h>

#include "Option.hpp"
#include <chrono>
#include <fort/utils/Defer.hpp>
#include <iomanip>
#include <ios>
#include <ratio>
#include <regex>
#include <string>

namespace fort {
namespace options {
namespace details {

class OptionTest : public testing::Test {};

TEST_F(OptionTest, BoolHaveEnforcedDefaultValueAndRequirement) {
	bool value{true};
	auto opt = Option<bool>(
	    {
	        .ShortFlag   = 0,
	        .Name        = "my-flag",
	        .Description = "turn my flag on",
	        .Required    = true,
	    },
	    value
	);
	EXPECT_FALSE(value);
	EXPECT_FALSE(opt.Required());
	EXPECT_FALSE(opt.Repeatable());
	EXPECT_EQ(opt.NumArgs(), 0);
}

TEST_F(OptionTest, BoolParsing) {
	bool value{false};
	auto opt = Option<bool>(
	    {
	        .ShortFlag   = 0,
	        .Name        = "my-flag",
	        .Description = "turn my flag on",
	        .Required    = true,
	    },
	    value
	);
	opt.Parse(std::nullopt);
	EXPECT_TRUE(value);

	opt.Parse("false");
	EXPECT_FALSE(value);

	opt.Parse("true");
	EXPECT_TRUE(value);

	try {
		opt.Parse("foo");
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag='foo'");
	}
}

TEST_F(OptionTest, IntParsing) {
	int  value{25};
	auto opt = Option<int>(
	    {
	        .ShortFlag   = 0,
	        .Name        = "my-flag",
	        .Description = "turn my flag on",
	    },
	    value
	);
	EXPECT_EQ(value, 25);
	ASSERT_EQ(opt.NumArgs(), 1);

	opt.Parse("42");
	EXPECT_EQ(value, 42);

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

TEST_F(OptionTest, FloatParsing) {
	float value{1.0};
	auto  opt = Option<float>(
        {
	         .ShortFlag   = 0,
	         .Name        = "my-flag",
	         .Description = "turn my flag on",
        },
        value
    );
	EXPECT_FLOAT_EQ(value, 1.0);
	ASSERT_EQ(opt.NumArgs(), 1);

	opt.Parse("42");
	EXPECT_FLOAT_EQ(value, 42.0);

	opt.Parse("-1e-4");
	EXPECT_FLOAT_EQ(value, -1e-4);

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

TEST_F(OptionTest, StringParsing) {
	std::string value{"something"};
	auto        opt = Option<std::string>(
        {
	               .ShortFlag   = 0,
	               .Name        = "my-flag",
	               .Description = "turn my flag on",
        },
        value
    );
	ASSERT_EQ(opt.NumArgs(), 1);
	EXPECT_EQ(value, "something");

	opt.Parse("the lazy dog jumps over the brown fox.");
	EXPECT_EQ(value, "the lazy dog jumps over the brown fox.");

	opt.Parse("");
	EXPECT_EQ(value, "");

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
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
		return out << std::setprecision(3) << (d.count() / 1000.0) << "us";
	}

	if (abs < SECOND) {
		return out << std::setprecision(3) << (d.count() / 1.0e6) << "ms";
	}

	if (abs < 60U * SECOND) {
		return out << std::setprecision(3) << (d.count() / 1.0e9) << "s";
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

TEST_F(OptionTest, CustomParsing) {
	Duration value{12345};
	auto     opt = Option<Duration>(
        {
	            .ShortFlag   = 0,
	            .Name        = "my-flag",
	            .Description = "turn my flag on",
        },
        value
    );
	ASSERT_EQ(opt.NumArgs(), 1);
	EXPECT_EQ(value, Duration{12345});

	opt.Parse("23.3ms");
	EXPECT_EQ(value, Duration{int64_t(233e5)});

	opt.Parse("4us");
	EXPECT_EQ(value, Duration{int64_t(4000)});

	opt.Parse("1h0m0s");
	EXPECT_EQ(
	    value,
	    std::chrono::duration_cast<Duration>(std::chrono::hours{1})
	);

	try {
		opt.Parse(std::nullopt);
		ADD_FAILURE() << "should throw a runtime_error";
	} catch (const std::exception &e) {
		EXPECT_STREQ(e.what(), "could not parse my-flag=''");
	}
}

} // namespace details
} // namespace options
} // namespace fort
