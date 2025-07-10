#include <cpptrace/from_current.hpp>

#include "fort/options/details/Option.hpp"
#include "fort/options/tests.hpp"
#include <fort/options/Options.hpp>

#include <gmock/gmock.h>

#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>

namespace fort {
namespace options {

class OptionsTest : public ::testing::Test {
protected:
	struct PID : public Group {
		float &K = AddOption<float>("K", "Proportional gain"); // required
		float &I = AddOption<float>("I", "Integral gain");     // required
		float &D = AddOption<float>("D", "Derivative gain");   // required
	};

	struct Opts : public Group {
		int &threshold = AddOption<int>("threshold,t", "an important threshold")
		                     .SetDefault(10); // optional

		PID &pid = AddSubgroup<PID>("pid", "the pid to set"); // subgroup

		std::vector<std::string> &includes = AddRepeatableOption<std::string>(
		    "I,include", "include directories"
		);
	};

	const static std::string USAGE;
};

const std::string OptionsTest::USAGE = R"x(Usage:
test [OPTIONS]

Runs a PID

Application Options:
  -t, --threshold   : an important threshold [default: 10]
  -I, --include     : include directories [default: []]

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

TEST_F(OptionsTest, ParsesArguments) {
	Opts opts;
	opts.SetDescription("Runs a PID");

	const char *argv[] = {
	    "tested",
	    "-t",
	    "32",
	    "--pid.K=42.0",
	    "--pid.I",
	    "0.001",
	    "--pid.D=62.5",
	    "-I",
	    "include",
	    "--include=vendor/include",
	    "foo",
	    "bar",
	};

	int argc = sizeof(argv) / sizeof(const char *);

	EXPECT_NO_THROW({ opts.ParseArguments(argc, argv); });

	EXPECT_EQ(opts.threshold, 32);
	EXPECT_EQ(
	    opts.includes,
	    std::vector<std::string>({"include", "vendor/include"})
	);
	EXPECT_FLOAT_EQ(opts.pid.K, 42.0);
	EXPECT_FLOAT_EQ(opts.pid.I, 0.001);
	EXPECT_FLOAT_EQ(opts.pid.D, 62.5);
	ASSERT_EQ(argc, 3);
	EXPECT_STREQ(argv[0], "tested");
	EXPECT_STREQ(argv[1], "foo");
	EXPECT_STREQ(argv[2], "bar");
}

TEST_F(OptionsTest, FailsRequiredArguments) {
	Opts opts;
	opts.SetDescription("Runs a PID");

	const char *argv[] = {
	    "tested",
	    "-t",
	    "32",
	};

	int argc = sizeof(argv) / sizeof(const char *);

	EXPECT_STDEXCEPT(
	    { opts.ParseArguments(argc, argv); },
	    std::runtime_error,
	    "missing required option 'pid.K'"
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

#ifdef CHARIS_OPTIONS_USE_FKYAML
TEST_F(OptionsTest, CanParseYAML) {
	std::string        raw = R"(pid:
  K: 42.0
  I: 0.001
  D: 0.625
threshold: 23
include:
  - foo
  - bar
)";
	std::istringstream iss(raw);
	Opts               opts;

	EXPECT_NO_THROW(opts.ParseYAML(iss));

	EXPECT_FLOAT_EQ(opts.pid.K, 42.0);
	EXPECT_FLOAT_EQ(opts.pid.I, 0.001);
	EXPECT_FLOAT_EQ(opts.pid.D, 0.625);
	EXPECT_EQ(opts.threshold, 23);
	EXPECT_EQ(opts.includes, std::vector<std::string>({"foo", "bar"}));
}
#endif

TEST_F(OptionsTest, CanParseFlags) {
	struct OptionWithFlags : public Group {
		bool &Flags = AddOption<bool>("flag,f", "A flag to set");
	};

	OptionWithFlags opts;
	EXPECT_NO_THROW({
		int         argc    = 1;
		const char *argv[1] = {"program"};
		opts.ParseArguments(argc, argv);
		EXPECT_FALSE(opts.Flags);
	});

	CPPTRACE_TRY {
		int         argc    = 2;
		const char *argv[2] = {"program", "--flag"};
		opts.ParseArguments(argc, argv);
		EXPECT_TRUE(opts.Flags);
	}
	CPPTRACE_CATCH(const std::exception &e) {
		ADD_FAILURE() << "Unexpected exception: " << e.what();
		cpptrace::from_current_exception().print();
	}

	CPPTRACE_TRY {
		int         argc    = 2;
		const char *argv[2] = {"program", "--flag=false"};
		opts.ParseArguments(argc, argv);
		EXPECT_FALSE(opts.Flags);
	}
	CPPTRACE_CATCH(const std::exception &e) {
		ADD_FAILURE() << "Unexpected exception: " << e.what();
		cpptrace::from_current_exception().print();
	}

	CPPTRACE_TRY {
		int         argc    = 2;
		const char *argv[2] = {"program", "--flag=true"};
		opts.ParseArguments(argc, argv);
		EXPECT_TRUE(opts.Flags);
	}
	CPPTRACE_CATCH(const std::exception &e) {
		ADD_FAILURE() << "Unexpected exception: " << e.what();
		cpptrace::from_current_exception().print();
	}

	try {
		int         argc    = 2;
		const char *argv[2] = {"program", "--flag=foo"};
		opts.ParseArguments(argc, argv);
		ADD_FAILURE() << "it should have thrown an error";
	} catch (const details::ParseError &e) {
		EXPECT_EQ(std::string(e.what()), "could not parse flag='foo'");
	} catch (const std::exception &e) {
		ADD_FAILURE() << "It throws unexpected exception: " << e.what();
	}
}

} // namespace options
} // namespace fort
