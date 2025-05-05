#pragma once

#include <cctype>
#include <locale>
#include <regex>
#include <stdexcept>
#include <string>
#include <tuple>

namespace fort {
namespace options {
namespace details {

static void checkName(const std::string &name) {
	static std::regex nameRx{"[a-zA-Z][a-zA-Z\\-_0-9]*"};
	if (std::regex_match(name, nameRx) == false) {
		throw std::invalid_argument{"invalid name '" + name + "'"};
	}
}

inline std::tuple<char, std::string>
parseDesignators(const std::string &designator) {
	constexpr static char DELIM = ',';

	auto pos = designator.find(DELIM);
	if (pos == std::string::npos) {
		if (designator.size() == 1) {
			if (std::isalpha(designator[0]) == false) {
				throw std::invalid_argument(
				    "invalid designator '" + designator + "'"
				);
			}
			return {designator[0], ""};
		}
		checkName(designator);
		return {0, designator};
	}

	if (designator.find(DELIM, pos + 1) != std::string::npos) {
		throw std::invalid_argument{
		    "invalid designators '" + designator +
		    "': at most two designators can be specified"};
	}

	if (designator.size() < 4 || (pos != 1 && pos != designator.size() - 2)) {
		throw std::invalid_argument{
		    "invalid designators '" + designator +
		    "': it should consist of a single letter and a long name, got "
		    "size " +
		    std::to_string(pos) + " and " +
		    std::to_string(designator.size() - pos - 1)};
	}

	char c = pos == 1 ? designator[0] : designator[pos + 1];
	if (std::isalpha(c, std::locale{}) == false) {
		throw std::invalid_argument{
		    "invalid designators '" + designator +
		    "': short flag should be a single letter"};
	}
	std::string name =
	    pos == 1 ? designator.substr(2) : designator.substr(0, pos);
	checkName(name);
	return {c, name};
}

} // namespace details
} // namespace options
} // namespace fort
