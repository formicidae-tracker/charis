#include <gtest/gtest.h>
#include <stdexcept>

#include "Designator.hpp"
#include "fort/options/tests.hpp"

namespace fort {
namespace options {
namespace details {

class DesignatorTest : public ::testing::Test {};

TEST_F(DesignatorTest, InvalidNumberOfDesignators) {

	EXPECT_STDEXCEPT(
	    { parseDesignators(""); },
	    std::invalid_argument,
	    "invalid name ''"
	);

	EXPECT_STDEXCEPT(
	    { parseDesignators("a,b,c"); },
	    std::invalid_argument,
	    "invalid designators 'a,b,c': at most two designators can be specified"
	);

	EXPECT_STDEXCEPT(
	    { parseDesignators("aa,"); },
	    std::invalid_argument,
	    "invalid designators 'aa,': it should consist of a single letter and a "
	    "long name, got size 2 and 0"
	);
	EXPECT_STDEXCEPT(
	    { parseDesignators(","); },
	    std::invalid_argument,
	    "invalid designators ',': it should consist of a single letter and a "
	    "long name, got size 0 and 0"
	);

	EXPECT_STDEXCEPT(
	    { parseDesignators("aa,bbb"); },
	    std::invalid_argument,
	    "invalid designators 'aa,bbb': it should consist of a single letter "
	    "and a "
	    "long name, got size 2 and 3"
	);

	EXPECT_STDEXCEPT(
	    { parseDesignators("a,"); },
	    std::invalid_argument,
	    "invalid designators 'a,': it should consist of a single letter "
	    "and a "
	    "long name, got size 1 and 0"
	);
}

TEST_F(DesignatorTest, InvalidShortFlags) {
	EXPECT_STDEXCEPT(
	    parseDesignators("foo,!"),
	    std::invalid_argument,
	    "invalid designators 'foo,!': short flag should be a single letter"
	);
}

TEST_F(DesignatorTest, InvalidDesignatorName) {
	const std::vector<std::string> data = {

	    "-foo",
	    "_bar",
	    "2foo",
	    "bazar{}"};

	for (const auto &d : data) {
		SCOPED_TRACE(d);
		EXPECT_STDEXCEPT(
		    parseDesignators(d),
		    std::invalid_argument,
		    ("invalid name '" + d + "'").c_str()
		);
	}
}

TEST_F(DesignatorTest, Parsing) {
	struct TestData {
		std::string                                  Designator;
		std::tuple<std::optional<char>, std::string> Expected;
	};

	std::vector<TestData> data = {
	    {"foo", {0, "foo"}},
	    {"f,foo", {'f', "foo"}},
	    {"foo,f", {'f', "foo"}},
	    {"foo-bar,F", {'F', "foo-bar"}},
	    {"f", {'f', ""}},
	};

	for (const auto &d : data) {
		SCOPED_TRACE(d.Designator);
		EXPECT_EQ(parseDesignators(d.Designator), d.Expected);
	}
}

} // namespace details
} // namespace options
} // namespace fort
