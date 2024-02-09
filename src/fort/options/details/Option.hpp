#pragma once

#include "Traits.hpp"
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

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
	using Ptr = std::unique_ptr<OptionBase>;

	OptionBase(OptionData && args)
	    : OptionData{std::move(args)} {}

	virtual ~OptionBase() = default;

	virtual void Parse(const std::optional<std::string> &) = 0;
	virtual void Format(std::ostream &out) const   = 0;

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

template <typename T> struct Args {
	char        ShortFlag;
	std::string Name;
	std::string Description;
	T	      &Value;
	bool        Required;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class Option : public OptionBase {
public:
	Option(Args<T> &&args)
	    : OptionBase{{
	          .ShortFlag   = args.ShortFlag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .Required    = args.Required,
	          .Repeatable  = false,
	      }}
	    , d_value{args.Value} {}

	void Parse(const std::optional<std::string> &value) override {
		if (NumArgs() > 0 && !value.has_value()) {
			throw ParseError{Name(), ""};
		}
		OptionBase::Parse<T>(value.value_or(""), d_value);
	}

	void Format(std::ostream &out) const override {
		out << d_value;
	}

private:
	T &d_value;
};

template <> class Option<bool> : public OptionBase {
public:
	Option(Args<bool> &&args)
	    : OptionBase{{
	          .ShortFlag   = args.ShortFlag,
	          .Name        = args.Name,
	          .Description = args.Description,
	          .NumArgs     = 0,
	          .Required    = false,
	          .Repeatable  = false,

	      }}
	    , d_value{args.Value} {
		d_value = false;
	}

	void Parse(const std::optional<std::string> &value) override {
		if (value.has_value() == false) {
			d_value = true;
			return;
		}

		if (value.value() == "true") {
			d_value = true;
		} else if (value.value() == "false") {
			d_value = false;
		} else {
			throw ParseError{Name(), value.value()};
		}
	}

	void Format(std::ostream &out) const override {
		if (d_value) {
			out << "true";
		} else {
			out << "false";
		}
	}

private:
	bool &d_value;
};

template <typename T, std::enable_if_t<is_optionable_v<T>> * = nullptr>
class RepeatableOption : public OptionBase {};

    } // namespace details
} // namespace options
} // namespace fort
