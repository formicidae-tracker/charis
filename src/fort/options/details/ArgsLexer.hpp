#pragma once

#include <cctype>
#include <locale>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace fort {
namespace options {
namespace details {

enum class TokenType {
	IDENTIFIER            = 0,
	VALUE                 = 1,
	IDENTIFIER_WITH_VALUE = 2,
};

struct ArgToken {
	TokenType   Type;
	std::string Value;
};

inline std::vector<ArgToken> lexArguments(int argc, char **argv) {

	constexpr static auto invalid_flag = [](const std::string &flag,
	                                        int                pos,
	                                        const std::string &reason = "") {
		if (reason.size() == 0) {
			return std::runtime_error{
			    "invalid flag '" + flag + "' at index " + std::to_string(pos)};
		}
		return std::runtime_error{
		    "invalid flag '" + flag + "' at index " + std::to_string(pos) +
		    ": " + reason};
	};

	std::vector<ArgToken> res;
	res.reserve(argc);
	for (int i = 0; i < argc; ++i) {
		std::string arg{argv[i]}; // parse the C string

		if (arg.size() == 0 || arg[0] != '-') {
			res.emplace_back(ArgToken{.Type = TokenType::VALUE, .Value = arg});
			continue;
		}

		if (arg.size() == 1) {
			throw invalid_flag(arg, i);
		}

		if (arg[1] != '-') {
			for (const char c : arg.substr(1)) {
				static std::locale loc{};
				if (std::isalpha(c, loc) == false) {
					throw invalid_flag(
					    arg,
					    i,
					    "short flags should only contain letters"
					);
				}
				res.emplace_back(ArgToken{
				    .Type  = TokenType::IDENTIFIER,
				    // I hate you C++, I need to use () and not {} to call
				    // the right constructor
				    .Value = std::string(1, c),
				});
			}
			continue;
		}
		const static std::regex flagRx{"[a-zA-Z][a-zA-Z\\-_0-9\\.]*"};

		if (arg.size() == 2) {
			res.emplace_back(ArgToken{.Type = TokenType::IDENTIFIER});
			continue;
		}

		auto equalPos = arg.find('=');

		if (equalPos == std::string::npos) {
			std::string ident = arg.substr(2);
			if (std::regex_match(ident, flagRx) == false) {
				throw invalid_flag(
				    arg,
				    i,
				    "invalid long flag name '" + ident + "'"
				);
			}
			res.emplace_back(
			    ArgToken{.Type = TokenType::IDENTIFIER, .Value = ident}
			);
			continue;
		}
		if (arg.substr(equalPos + 1).find('=') != std::string::npos) {
			throw invalid_flag(arg, i, "contains more than one '='");
		}

		std::string ident = arg.substr(2, equalPos - 2);
		if (std::regex_match(ident, flagRx) == false) {
			throw invalid_flag(
			    arg,
			    i,
			    "invalid long flag name '" + ident + "'"
			);
		}
		res.emplace_back(
		    ArgToken{.Type = TokenType::IDENTIFIER_WITH_VALUE, .Value = ident}
		);
		res.emplace_back(ArgToken{
		    .Type  = TokenType::VALUE,
		    .Value = arg.substr(equalPos + 1),
		});
	}

	return res;
}
} // namespace details
} // namespace options
} // namespace fort
