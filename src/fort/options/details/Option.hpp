#pragma once

#include "Traits.hpp"
#include <ios>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <fort/utils/Defer.hpp>

namespace fort {
namespace options {
namespace details {

struct OptionData {
	char        ShortFlag;
	std::string Name, Description;
	size_t      NumArgs = 1;
	bool        Required;
	bool        Repeatable;
};

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string &name, const std::string &value)
	    : std::runtime_error{"could not parse " + name + "='" + value + "'"} {}
};

class OptionBase : private OptionData {
public:
	OptionBase(OptionData &&args)
	    : OptionData{std::move(args)} {}

	virtual ~OptionBase() = default;

	virtual void Parse(const std::optional<std::string> &) = 0;
	virtual void Format(std::ostream &out) const           = 0;

	inline const std::string &Name() const noexcept {
		return OptionData::Name;
	}

	inline const std::string &Description() const noexcept {
		return OptionData::Description;
	}

	inline char Short() const noexcept {
		return OptionData::ShortFlag;
	}

	bool Required() const noexcept {
		return OptionData::Required;
	}

	bool Repeatable() const noexcept {
		return OptionData::Repeatable;
	}

	size_t NumArgs() const noexcept {
		return OptionData::NumArgs;
	}

protected:
	template <typename T>
	inline void Parse(const std::string &value, T &res) const {
		std::istringstream iss(value);
		iss >> res;
		if (iss.fail()) {
			throw ParseError{OptionData::Name, value};
		}
	}
};

template <>
inline void OptionBase::Parse<std::string>(
    const std::string &value, std::string &res
) const {
	res = value;
}

template <>
inline void OptionBase::Parse<bool>(const std::string &value, bool &res) const {
	if (value.empty() || value == "true") {
		res = true;
	} else if (value == "false") {
		res = false;
	} else {
		throw ParseError{Name(), value};
	}
}

struct OptionArgs {
	char        ShortFlag;
	std::string Name;
	std::string Description;
	bool        Required;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class Option : public OptionBase {
public:
	Option(OptionArgs &&args, T &value)
	    : OptionBase{{
	          .ShortFlag   = args.ShortFlag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .NumArgs     = std::is_same_v<T, bool> ? 0 : 1,
	          .Required    = std::is_same_v<T, bool> ? false : args.Required,
	          .Repeatable  = false,
	      }}
	    , d_value{value} {
		if constexpr (std::is_same_v<T, bool>) {
			d_value = false;
		}
	}

	void Parse(const std::optional<std::string> &value) override {
		if (NumArgs() > 0 && value.has_value() == false) {
			throw ParseError{Name(), ""};
		}
		OptionBase::Parse<T>(value.value_or(""), d_value);
	}

	void Format(std::ostream &out) const override {
		auto f = out.flags();
		defer {
			out.flags(f);
		};
		out << std::boolalpha << d_value;
	}

private:
	T &d_value;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class RepeatableOption : public OptionBase {
public:
	RepeatableOption(OptionArgs &&args, std::vector<T> &value)
	    : OptionBase{{
	          .ShortFlag   = args.ShortFlag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .NumArgs     = std::is_same_v<T, bool> ? 0 : 1,
	          .Required    = std::is_same_v<T, bool> ? false : args.Required,
	          .Repeatable  = false,
	      }}
	    , d_value{value} {}

	void Parse(const std::optional<std::string> &value) override {
		if (NumArgs() > 0 && value.has_value() == false) {
			throw ParseError{Name(), ""};
		}

		OptionBase::Parse<T>(value.value_or(""), d_value.emplace_back());
	}

	void Format(std::ostream &out) const override {
		auto f = out.flags();
		defer {
			out.flags(f);
		};
		std::string sep = "";
		out << std::boolalpha << "[";
		for (const auto &v : d_value) {
			out << sep << v;
			sep = ", ";
		}
		out << "]";
	}

private:
	std::vector<T> &d_value;
};

} // namespace details
} // namespace options
} // namespace fort
