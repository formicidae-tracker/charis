#pragma once

#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <fort/options/details/Designator.hpp>
#include <fort/options/details/Option.hpp>
#include <fort/options/details/Traits.hpp>

namespace fort {
namespace options {
constexpr char NO_SHORT = 0;

class Group {
public:
	Group()
	    : d_parent{std::nullopt}
	    , d_shortFlags{ValuesByShort{}} {}

	virtual ~Group() = default;

	template <
	    typename T,
	    std::enable_if_t<details::is_optionable_v<T>> * = nullptr>

	details::Option<T> &AddOption(
	    const std::string &designator,
	    const std::string &description,
	    std::optional<T>   implicit = std::nullopt
	) {
		auto opt = std::make_shared<details::Option<T>>(
		    checkArgs(designator, description),
		    implicit
		);
		pushOption(opt);
		return *opt;
	}

	template <
	    typename T,
	    std::enable_if_t<details::is_specialization_v<T, std::vector>> * =
	        nullptr>
	details::Option<T> &
	AddOption(const std::string &designator, const std::string &description) {
		auto opt = std::make_shared<details::RepeatableOption<T>>(
		    checkArgs(designator, description)
		);
		return pushOption(opt);
	}

	template <
	    typename T,
	    std::enable_if_t<std::is_base_of_v<Group, T>, void *> = nullptr>
	T &AddSubgroup(const std::string &name, const std::string &description) {
		details::checkName(name);
		if (d_subgroups.count(name) != 0) {
			throw std::invalid_argument("group '" + name + "' already exist");
		}

		T &res =
		    *(d_subgroups.insert({name, std::make_unique<T>()}).first->second);

		for (const auto &[flag, opt] : res.d_shortFlags.value()) {
			if (opt->Name.size() != 0) {
				throw std::invalid_argument{
				    "subgroup option '" + name + "." + opt->Name +
				    " cannot have short flag '" + std::string(1, flag) + "'"};
			}
			opt->Name = std::string(1, flag);
			res.d_longFlags.insert({opt->Name, opt});
		}

		res.d_shortFlags  = std::nullopt;
		res.d_name        = name;
		res.d_description = description;
		res.d_parent      = this;

		return res;
	}

	template <typename T> operator T &() {
		return *this;
	}

	template <typename T> operator T() {}

	void Set(const std::string &name, const std::optional<std::string> &value) {
		if (name.size() == 1 && d_shortFlags.has_value()) {
			auto opt = d_shortFlags.value().find(name[0]);
			if (opt == d_shortFlags.value().end()) {
				throw std::out_of_range{"unknown short flag '" + name + "'"};
			}
			opt->second->Parse(value);
			return;
		}

		auto pos = name.find('.');
		if (pos == std::string::npos) {
			auto subgroupName = name.substr(0, pos);
			auto subgroup     = d_subgroups.find(subgroupName);
			if (subgroup == d_subgroups.end()) {
				throw std::out_of_range{"unknown option '" + name + "'"};
			}
			subgroup->second->Set(name.substr(pos + 1), value);
			return;
		}

		auto opt = d_longFlags.find(name);
		if (opt == d_longFlags.end()) {
			throw std::out_of_range{"unknown option '" + name + "'"};
		}
		opt->second->Parse(value);
	}

	void ParseArguments(int argc, const char **argv) {
		throw std::runtime_error("not yet implemented");
	}

	void ParseYAML(const std::string &filename) {
		throw std::runtime_error("not yet implemented");
	}

	void ParseINI(const std::string &filename) {
		throw std::runtime_error("not yet implemented");
	}

private:
	using OptionPtr = std::shared_ptr<details::OptionBase>;
	using GroupPtr  = std::unique_ptr<Group>;

	using ValuesByShort = std::map<char, OptionPtr>;
	using ValuesByLong  = std::map<std::string, OptionPtr>;

	std::string fullOptionName(const std::string &name) const {
		return prefix() + name;
	}

	std::string prefix() const {
		if (d_parent.has_value()) {
			return d_parent.value()->prefix() + d_name + ".";
		}
		return "";
	}

	details::OptionArgs checkArgs(
	    const std::string &designator, const std::string &description
	) const {
		if (description.empty()) {
			throw std::invalid_argument{"Description cannot be empty"};
		}

		const auto [flag, name] = details::parseDesignators(designator);

		if (d_longFlags.count(name) > 0) {
			throw std::invalid_argument{
			    "option '" + name + "' already specified",
			};
		}

		if (d_shortFlags.has_value() == false) {
			if (flag != 0) {
				throw std::runtime_error{"cannot set short flag in subgroup"};
			}
		} else if (flag != 0) {
			auto sh = d_shortFlags.value().find(flag);
			if (sh != d_shortFlags.value().end()) {
				throw std::invalid_argument{
				    "short flag '" + std::string(1, flag) +
				        "' already used by option '" + sh->second->Name + "'",
				};
			}
		}

		return {
		    .ShortFlag   = flag,
		    .Name        = name,
		    .Description = description,
		};
	}

	template <typename T>
	details::Option<T> &pushOption(std::shared_ptr<details::Option<T>> option) {
		if (option->Name.size() > 0) {
			d_longFlags[option->Name] = option;
		}
		if (option->ShortFlag != 0) {
			d_shortFlags.value().insert({option->ShortFlag, option});
		}

		return *option;
	}

	std::string d_name;
	std::string d_description;

	ValuesByLong d_longFlags;

	std::optional<Group *>          d_parent;
	std::optional<ValuesByShort>    d_shortFlags;
	std::map<std::string, GroupPtr> d_subgroups;
};

} // namespace options
} // namespace fort
