#include <fort/options/Options.hpp>
#include <gtest/gtest.h>
#include <stdexcept>

namespace fort {
namespace options {

class OptionsTest : public ::testing::Test {
protected:
	void SetUp() {
		options = std::make_shared<OptionGroup>("", "", nullptr);
	}

	std::shared_ptr<OptionGroup> options;
};

TEST_F(OptionsTest, NameChecking) {
	EXPECT_NO_THROW({ options->AddGroup("sub-Group_43", ""); });
	EXPECT_THROW({ options->AddGroup("foo+", ""); }, std::invalid_argument);
	EXPECT_THROW({ options->AddGroup(" bar", ""); }, std::invalid_argument);
	EXPECT_THROW({ options->AddGroup("=", ""); }, std::invalid_argument);
}

TEST_F(OptionsTest, DuplicateOptionChecking) {
	int value;
	EXPECT_NO_THROW({ options->AddOption({.Name = "foo"}, value); });
	EXPECT_THROW(
	    { options->AddOption({.Name = "foo"}, value); },
	    std::invalid_argument
	);
}

} // namespace options
} // namespace fort
