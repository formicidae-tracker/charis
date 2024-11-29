#include "fort/options/tests.hpp"
#include <fort/options/Options.hpp>

#include <gmock/gmock.h>

#include <sstream>
#include <stdexcept>

namespace fort {
namespace options {

class OptionsTest : public ::testing::Test {
protected:
	struct PID : public Group {
		float &K = AddOption<float>("K", "Proportional gain");
		float &I = AddOption<float>("I", "Integral gain");
		float &D = AddOption<float>("D", "Derivative gain");
	};

	struct Opts : public Group {
		int &threshold = AddOption<int>("threshold,t", "an important threshold")
		                     .SetDefault(10);

		PID &pid = AddSubgroup<PID>("pid", "the pid to set");
	};

	const static std::string USAGE;
};

const std::string OptionsTest::USAGE = R"x(Usage:
test [OPTIONS]

Runs a PID

Application Options:
  -t, --threshold   : an important threshold [default: 10]

pid: the pid to set
      --pid.K       : Proportional gain [required]
      --pid.I       : Integral gain [required]
      --pid.D       : Derivative gain [required]

Help Options:
  -h, --help        : displays this help message
)x";

TEST_F(OptionsTest, SetValues) {
	Opts opts;

	EXPECT_NO_THROW({
		opts.Set("t", "1");
		opts.Set("threshold", "2");
		opts.Set("pid.K", "3");
		opts.Set("pid.I", "4");
		opts.Set("pid.D", "5");
	});
	EXPECT_EQ(opts.threshold, 2);
	EXPECT_FLOAT_EQ(opts.pid.K, 3);
	EXPECT_FLOAT_EQ(opts.pid.I, 4);
	EXPECT_FLOAT_EQ(opts.pid.D, 5);
}

TEST_F(OptionsTest, FormatUsage) {
	Opts opts;
	opts.SetDescription("Runs a PID");
	opts.SetName("test");
	std::ostringstream oss;
	opts.FormatUsage(oss);

	EXPECT_EQ(oss.str(), USAGE);
}

TEST_F(OptionsTest, HelpArgumentExits) {
	Opts opts;
	opts.SetDescription("Runs a PID");

	const char *argv[] = {"tested", "-h"};
	int         argc   = 2;

	EXPECT_EXIT(
	    { opts.ParseArguments(argc, argv); },
	    testing::ExitedWithCode(0),
	    ::testing::AllOf(
	        ::testing::ContainsRegex("tested \\[OPTIONS\\]"),
	        ::testing::ContainsRegex("Usage:")
	    )
	);
}

TEST_F(OptionsTest, NameChecking) {

	struct InvalidName : public Group {
		int &anInt =
		    AddOption<int>("[NOTAllowed]", "an int to save").SetDefault(42);
	};

	EXPECT_STDEXCEPT(
	    InvalidName{},
	    std::invalid_argument,
	    "invalid name '[NOTAllowed]'"
	);

	struct DuplicatedName : public Group {
		int &anInt = AddOption<int>("i,anInt", "an int to save").SetDefault(42);
		int &anInt2 =
		    AddOption<int>("I,anInt", "an int2 to save").SetDefault(42);
	};

	EXPECT_STDEXCEPT(
	    DuplicatedName{},
	    std::invalid_argument,
	    "option 'anInt' already specified"
	);

	struct Help : public Group {
		bool &help =
		    AddOption<bool>("h,help", "display this help").SetDefault(42);
	};

	EXPECT_STDEXCEPT(
	    Help{},
	    std::invalid_argument,
	    "-h/--help flag is reserved"
	);
}

} // namespace options
} // namespace fort
