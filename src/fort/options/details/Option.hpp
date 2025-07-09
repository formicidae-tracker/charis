#pragma once

#include <cctype>
#include <ios>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <fort/utils/Defer.hpp>

#include "Traits.hpp"
#include "config.hpp"

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif

#ifdef FORT_CHARIS_CAPITALIZE_DEFER
#define defer Defer
#endif

namespace fort {
namespace options {
namespace details {

struct OptionData {
	char        Flag;
	std::string Name, Description;
	size_t      NumArgs = 1;
	bool        Required;
	bool        Repeatable;
};

class ParseError : public std::runtime_error {
public:
	ParseError(const std::string &name, const std::string &value) noexcept
	    : std::runtime_error{"could not parse " + name + "='" + value + "'"} {}

	ParseError(
	    const std::string &name,
	    const std::string &value,
	    const std::string &reason
	) noexcept
	    : std::runtime_error{
	          "could not parse " + name + "='" + value + "': " + reason} {}
};

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
template <typename T, std::enable_if_t<std::is_enum_v<T>> * = nullptr>
class ParseEnumError : public ParseError {
public:
	ParseEnumError(const std::string &name, const std::string &value)
	    : ParseError{name, value, reason()} {}

private:
	static std::string reason() {
		constexpr auto    &entries = magic_enum::enum_values<T>();
		std::ostringstream oss;
		std::string        prefix{"['"};

		oss << "possible enum values are ";

		for (const auto &e : entries) {
			oss << prefix << magic_enum::enum_name(e);
			prefix = "', '";
		}

		oss << "']";
		return oss.str();
	}
};
#endif

class OptionBase : public OptionData {
public:
	OptionBase(OptionData &&args)
	    : OptionData{std::move(args)} {}

	virtual ~OptionBase() = default;

	virtual void Parse(const std::optional<std::string> &) = 0;
	virtual void Format(std::ostream &out) const           = 0;

	bool IsSet() const {
		return d_set;
	}

protected:
	template <typename T>
	inline void Parse(const std::string &value, T &res) const {
		if constexpr (std::is_enum_v<T>) {
#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
			auto v = magic_enum::enum_cast<T>(value);
			if (v.has_value() == false) {
				throw ParseEnumError<T>{this->Name, value};
			}
			res = v.value();
#else
			throw std::logic_error(
			    "Enum are not supported, please link with magic_enum"
			);
#endif

		} else {
			std::istringstream iss(value);
			iss >> res;
			if (iss.fail()) {
				throw ParseError{this->Name, value};
			}
		}
	}

	template <typename T>
	inline static void Format(std::ostream &out, const T &value) {
		if constexpr (std::is_enum_v<T>) {
#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
			out << magic_enum::enum_name(value);
#else
			throw std::logic_error(
			    "Enum are not supported, please link with magic_enum"
			);
#endif
		} else {
			auto f = out.flags();
			defer {
				out.flags(f);
			};
			out << std::boolalpha << value;
		}
	}

	bool d_set = false;
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
		throw ParseError{Name, value};
	}
}

struct OptionArgs {
	char        Flag = 0;
	std::string Name;
	std::string Description;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class Option : public OptionBase {
public:
	Option(OptionArgs &&args, std::optional<T> implicit = std::nullopt)
	    : OptionBase{{
	          .Flag        = args.Flag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .NumArgs     = std::is_same_v<T, bool> ? 0 : 1,
	          .Required    = std::is_same_v<T, bool> ? false : true,
	          .Repeatable  = false,
	      }} {
		if (implicit.has_value()) {
			this->Value = implicit.value();
			return;
		}

		if constexpr (std::is_fundamental_v<T>) {
			Value = 0;
		}

#ifdef CHARIS_OPTIONS_USE_MAGIC_ENUM
		if constexpr (std::is_enum_v<T>) {
			if constexpr (magic_enum::enum_count<T>() > 0) {
				Value = magic_enum::enum_values<T>()[0];
			}
		}
#endif
	}

	void Parse(const std::optional<std::string> &value) override {
		if (this->NumArgs > 0 && value.has_value() == false) {
			throw ParseError{this->Name, ""};
		}
		OptionBase::Parse<T>(value.value_or(""), this->Value);
		d_set = true;
	}

	void Format(std::ostream &out) const override {
		OptionBase::Format<T>(out, Value);
	}

	Option &SetDefault(const T &value) {
		this->Required = false;
		this->Value    = value;
		return *this;
	}

	operator T &() {
		return Value;
	}

	template <typename U> operator U() {}

	T Value;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class RepeatableOption : public OptionBase {
public:
	RepeatableOption(OptionArgs &&args)
	    : OptionBase{{
	          .Flag        = args.Flag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .NumArgs     = std::is_same_v<T, bool> ? 0 : 1,
	          .Required    = false,
	          .Repeatable  = true,
	      }} {}

	void Parse(const std::optional<std::string> &value) override {
		if (this->NumArgs > 0 && value.has_value() == false) {
			throw ParseError{this->Name, ""};
		}

		OptionBase::Parse<T>(value.value_or(""), this->value.emplace_back());
	}

	void Format(std::ostream &out) const override {
		std::string sep = "";
		out << std::boolalpha << "[";
		for (const auto &v : value) {
			out << sep;
			OptionBase::Format<T>(out, v);
			sep = ", ";
		}
		out << "]";
	}

	void SetDefault(const std::vector<T> &value) {
		this->Required = false;
		this->value    = value;
	}

	operator std::vector<T> &() {
		return this->value;
	}

	std::vector<T> value;
};

} // namespace details
} // namespace options
} // namespace fort

#ifdef FORT_CHARIS_CAPITALIZE_DEFER
#undef defer
#endif
