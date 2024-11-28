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

	Group(
	    const std::string &name, const std::string &description, Group *parent
	)
	    : d_name{name}
	    , d_description{description} {

		if (name.empty()) {
			throw std::invalid_argument{
			    "name could not be empty for child group"};
		}
		d_parent     = parent;
		d_shortFlags = std::nullopt;
	}

	virtual ~Group() = default;

	template <
	    typename T,
	    std::enable_if_t<details::is_optionable_v<T>> * = nullptr>

	details::Option<T> &AddOption(
	    const std::string &designator,
	    const std::string &description,
	    std::optional<T>   implicit = std::nullopt
	) {
		auto opt = std::make_unique<details::Option<T>>(
		    checkArgs(designator, description),
		    implicit
		);
		return pushOption(std::move(opt));
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
		pushOption(opt);
		return opt;
	}

	template <typename T, std::enable_if_t<std::is_base_of_v<Group, T>>>
	T &AddSubgroup(const std::string &name, const std::string &description) {
		details::checkName(name);
		if (d_subgroups.count(name) != 0) {
			throw std::invalid_argument("group '" + name + "' already exist");
		}

		auto group = std::make_shared<T>(name, description, this);

		d_subgroups.insert(name, group);

		return *group;
	}

	template <typename T> operator T &() {
		return *this;
	}

	template <typename T> operator T() {}

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
	using OptionPtr = std::unique_ptr<details::OptionBase>;
	using GroupPtr  = std::unique_ptr<Group>;

	using ValuesByShort =
	    std::map<char, std::pair<Group *, details::OptionBase *>>;
	using ValuesByLong = std::map<std::string, OptionPtr>;

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

		const auto [shortName, longName] =
		    details::parseDesignators(designator);

		if (d_longFlags.count(longName) > 0) {
			throw std::invalid_argument{
			    "option '" + fullOptionName(longName) + "' already specified",
			};
		}

		auto [parent, option] = this->findShort(shortName.value_or(0));

		if (shortName.has_value() && option != nullptr) {
			throw std::invalid_argument{
			    "short flag '" + std::string{1, shortName.value()} +
			        "' already used by option '" +
			        parent->fullOptionName(option->Name()) + "'",
			};
		}

		return {
		    .ShortFlag   = shortName,
		    .Name        = longName,
		    .Description = description,
		};
	}

	std::tuple<Group *, details::OptionBase *> findShort(char flag) const {
		if (d_parent.has_value()) {
			return d_parent.value()->findShort(flag);
		} else if (d_shortFlags.has_value()) {
			try {
				return d_shortFlags.value().at(flag);
			} catch (const std::exception &) {
				return {nullptr, nullptr};
			}
		} else {
			throw std::logic_error{"invalid group hierarchy"};
		}
	}

	template <typename T>
	details::Option<T> &pushOption(std::unique_ptr<details::Option<T>> option) {
		auto opt                    = option.get();
		d_longFlags[option->Name()] = std::move(option);
		mayPushShort(this, opt);
		return *opt;
	}

	void mayPushShort(Group *owner, details::OptionBase *option) {
		if (option->Short().has_value() == false) {
			return;
		}

		if (d_parent.has_value()) {
			d_parent.value()->mayPushShort(owner, option);
		} else if (d_shortFlags.has_value()) {
			d_shortFlags.value()[option->Short().value()] = {owner, option};
		} else {
			throw std::logic_error{"invalid group hierarchy"};
		}
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
