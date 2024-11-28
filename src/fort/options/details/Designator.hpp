#pragma once

#include <tuple>

namespace fort {
namespace options {
namespace details {
inline std::tuple<std::optional<char>, std::string>
parseDesignator(const std::string &designator) {
	std::vector<std::string> parts;
	parts.reserve(2);
	constexpr static char DELIM = ',';

	auto pos = designator.find(DELIM);
	if (pos == std::string::npos) {
		checkName(designator);
		return {std::nullopt, designator};
	}

	if (designator.find(DELIM, pos + 1) != std::string::npos) {
		throw std::invalid_argument(
		    "invalid designator '" + designator +
		    "': at most two name can be specified"
		);
	}

	if (pos != 1 && pos != designator.size() - 2) {
		throw std::invalid_argument("invalid des");
	}
}

} // namespace details
} // namespace options
} // namespace fort
