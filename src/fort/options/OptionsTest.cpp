#include "fort/options/tests.hpp"
#include <fort/options/Options.hpp>
#include <gtest/gtest.h>
#include <stdexcept>

namespace fort {
namespace options {

class SimpleOptionsTest : public ::testing::Test {
protected:
	struct Simple : public Group {
		int &threshold =
		    AddOption<int>("threshold,t", "an important threshold");
	};
};

TEST_F(SimpleOptionsTest, Simple) {

	const char *argv[3] = {"coucou", "--threshold=3"};

	Simple opts;

	EXPECT_THROW({ opts.ParseArguments(1, argv); }, std::runtime_error);
	EXPECT_EQ(opts.threshold, 0);
}

TEST_F(SimpleOptionsTest, NameChecking) {

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
}

} // namespace options
} // namespace fort
