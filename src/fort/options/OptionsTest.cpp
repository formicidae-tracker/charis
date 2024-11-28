#include <fort/options/Options.hpp>
#include <gtest/gtest.h>
#include <stdexcept>

namespace fort {
namespace options {

class OptionsTest : public ::testing::Test {};

TEST_F(OptionsTest, NameChecking) {

	struct InvalidName : public Group {
		int &anInt =
		    AddOption<int>("[NOTAllowed]", "an int to save").SetDefault(42);
	};
}

    // TEST_F(OptionsTest, DuplicateOptionChecking) {
	// 	int value;
	// 	EXPECT_NO_THROW({ options->AddOption({.Name = "foo"}, value); });
	// 	EXPECT_THROW(
	// 	    { options->AddOption({.Name = "foo"}, value); },
	// 	    std::invalid_argument
	// 	);
	// }

} // namespace options
} // namespace fort
