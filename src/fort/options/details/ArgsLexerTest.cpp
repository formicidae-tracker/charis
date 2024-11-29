#include <gmock/gmock.h>

#include <fort/options/details/ArgsLexer.hpp>
#include <fort/options/tests.hpp>

namespace fort {
namespace options {
namespace details {

bool operator==(const ArgToken &a, const ArgToken &b) {
	return a.Type == b.Type && a.Value == b.Value;
}

std::ostream &operator<<(std::ostream &out, const ArgToken &t) {
	return out << "ArgToken{ .Type="
	           << (t.Type == TokenType::IDENTIFIER ? "IDENTIFIER" : "VALUE")
	           << ", .Value=\"" << t.Value << "\"}";
}

class ArgLexerTest : public ::testing::Test {};

TEST_F(ArgLexerTest, LexManyThing) {
	const char *argv[] = {
	    "progName",
	    "-f",
	    "-afh",
	    "--foo",
	    "bar",
	    "--some.other=something",
	    "unamed",
	    "--",
	    "boo bah baz",
	};

	std::vector<ArgToken> expected = {
	    {TokenType::VALUE, "progName"},
	    {TokenType::IDENTIFIER, "f"},
	    {TokenType::IDENTIFIER, "a"},
	    {TokenType::IDENTIFIER, "f"},
	    {TokenType::IDENTIFIER, "h"},
	    {TokenType::IDENTIFIER, "foo"},
	    {TokenType::VALUE, "bar"},
	    {TokenType::IDENTIFIER_WITH_VALUE, "some.other"},
	    {TokenType::VALUE, "something"},
	    {TokenType::VALUE, "unamed"},
	    {},
	    {TokenType::VALUE, "boo bah baz"},
	};

	using ::testing::ElementsAreArray;

	EXPECT_NO_THROW({
		EXPECT_THAT(
		    lexArguments(sizeof(argv) / sizeof(const char *), (char **)argv),
		    ElementsAreArray(expected)
		);
	});
}

TEST_F(ArgLexerTest, NoEmptyFlag) {
	const char *argv[] = {"-"};
	EXPECT_STDEXCEPT(
	    lexArguments(1, (char **)argv),
	    std::runtime_error,
	    "invalid flag '-' at index 0"
	);
}

TEST_F(ArgLexerTest, InvalidShortFlags) {
	const char *argv[] = {"-ab!"};
	EXPECT_STDEXCEPT(
	    lexArguments(1, (char **)argv),
	    std::runtime_error,
	    "invalid flag '-ab!' at index 0: short flags should only contain "
	    "letters"
	);
}

TEST_F(ArgLexerTest, LongFlagName) {
	const char *argv[] = {"--someInvalid[Name]"};
	EXPECT_STDEXCEPT(
	    lexArguments(1, (char **)argv),
	    std::runtime_error,
	    "invalid flag '--someInvalid[Name]' at index 0: invalid long flag name "
	    "'someInvalid[Name]'"
	);

	argv[0] = "--someInvalid[Name]=foo";
	EXPECT_STDEXCEPT(
	    lexArguments(1, (char **)argv),
	    std::runtime_error,
	    "invalid flag '--someInvalid[Name]=foo' at index 0: invalid long flag "
	    "name 'someInvalid[Name]'"
	);
}

TEST_F(ArgLexerTest, MultipleEqualFails) {
	const char *argv[] = {"--some=foo=bar"};
	EXPECT_STDEXCEPT(
	    lexArguments(1, (char **)argv),
	    std::runtime_error,
	    "invalid flag '--some=foo=bar' at index 0: contains more than one '='"
	);
}

} // namespace details
} // namespace options
} // namespace fort
