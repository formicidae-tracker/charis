#include "fort/options/tests.hpp"
#include <fort/options/Options.hpp>
#include <gtest/gtest.h>
#include <stdexcept>

namespace fort {
namespace options {

class OptionsTest : public ::testing::Test {
protected:
	struct PID : public Group {
		float &K = AddOption<float>("K_gain", "Proportional gain");
		float &I = AddOption<float>("I_gain", "Integral gain");
		float &D = AddOption<float>("D_gain", "Derivative gain");
	};

	struct Opts : public Group {
		int &threshold =
		    AddOption<int>("threshold,t", "an important threshold");

		PID &pid = AddSubgroup<PID>("pid", "the pid to set");
	};
};

TEST_F(OptionsTest, SetValues) {}

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
}

} // namespace options
} // namespace fort
