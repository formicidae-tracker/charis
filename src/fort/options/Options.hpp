#pragma once

#include <fort/options/details/Option.hpp>
#include <fort/options/details/Traits.hpp>
#include <libavcodec/avcodec.h>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fort {
namespace options {
constexpr char NO_SHORT = 0;

using OptionArgs = details::OptionArgs;

class OptionGroup : public std::enable_shared_from_this<OptionGroup> {
public:
	OptionGroup(
	    const std::string           &name,
	    const std::string           &description,
	    std::shared_ptr<OptionGroup> parent = nullptr
	)
	    : d_name{name}
	    , d_description{description} {
		if (parent) {
			if (name.empty()) {
				throw std::invalid_argument{
				    "name could not be empty for child group"};
			}
			d_parent     = parent;
			d_shortFlags = std::nullopt;
		} else {
			d_name       = "";
			d_shortFlags = ValuesByShort{};
			d_parent     = std::nullopt;
		}
	}

	virtual ~OptionGroup() = default;

	template <
	    typename T,
	    std::enable_if_t<details::is_optionable_v<T>> * = nullptr>
	void AddOption(OptionArgs &&args, T &value) {
		checkArgs(args);
		auto opt = std::make_shared<details::Option<T>>(std::move(args), value);
		pushOption(opt);
	}

	template <
	    typename T,
	    std::enable_if_t<details::is_specialization_v<T, std::vector>> * =
	        nullptr>
	void AddOption(OptionArgs &&args, T &value) {
		checkArgs(args);
		auto opt = std::make_shared<details::RepeatableOption<T>>(
		    std::move(args),
		    value
		);
		pushOption(opt);
	}

	std::shared_ptr<OptionGroup>
	AddGroup(const std::string &name, const std::string &description) {
		checkName(name);
		if (d_subgroups.count(name) != 0) {
			return d_subgroups.at(name);
		}

		auto res = std::make_shared<OptionGroup>(
		    name,
		    description,
		    shared_from_this()
		);

		d_subgroups[name] = res;
		return res;
	}

private:
	using OptionPtr = std::shared_ptr<details::OptionBase>;

	using GroupPtr     = std::shared_ptr<OptionGroup>;
	using GroupWeakPtr = std::weak_ptr<OptionGroup>;

	using ValuesByShort = std::map<char, std::pair<GroupPtr, OptionPtr>>;
	using ValuesByLong  = std::map<std::string, OptionPtr>;

	std::string fullOptionName(const std::string &name) const {
		return prefix() + name;
	}

	std::string prefix() const {
		if (d_parent.has_value()) {
			return d_parent->lock()->prefix() + d_name + ".";
		}
		return "";
	}

	void checkArgs(const OptionArgs &args) const {
		checkName(args.Name);

		if (d_longFlags.count(args.Name) > 0) {
			throw std::invalid_argument{
			    "option '" + fullOptionName(args.Name) + "' already specified",
			};
		}

		auto [parent, option] = this->findShort(args.ShortFlag);

		if (args.ShortFlag != NO_SHORT && option) {
			throw std::invalid_argument{
			    "short flag '" + std::string{1, args.ShortFlag} +
			        "' already used by option '" +
			        parent->fullOptionName(option->Name()) + "'",
			};
		}
	}

	std::tuple<GroupPtr, OptionPtr> findShort(char flag) const {
		if (d_parent.has_value()) {
			return d_parent->lock()->findShort(flag);
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

	static void checkName(const std::string &name) {
		static std::regex nameRx{"[a-zA-Z\\-_0-9]+"};
		if (std::regex_match(name, nameRx) == false) {
			throw std::invalid_argument{"invalid name '" + name + "'"};
		}
	}

	void pushOption(OptionPtr option) {
		d_longFlags[option->Name()] = option;
		mayPushShort(shared_from_this(), option);
	}

	void mayPushShort(const GroupPtr &owner, const OptionPtr &option) {
		if (option->Short() == NO_SHORT) {
			return;
		}

		if (d_parent.has_value()) {
			d_parent->lock()->mayPushShort(owner, option);
		} else if (d_shortFlags.has_value()) {
			d_shortFlags.value()[option->Short()] = {owner, option};
		} else {
			throw std::logic_error{"invalid group hierarchy"};
		}
	}

	std::string d_name;
	std::string d_description;

	ValuesByLong d_longFlags;

	std::optional<GroupWeakPtr>  d_parent;
	std::optional<ValuesByShort> d_shortFlags;

	std::map<std::string, GroupPtr> d_subgroups;
};

class OptionParser : public OptionGroup {
public:
	using Ptr = std::shared_ptr<OptionParser>;

	static Ptr Create(const std::string &name, const std::string &description) {
		auto res = new OptionParser(name, description);
		return Ptr{res};
	}

	std::vector<std::string> Parse(int argc, const char **argv);

	void ParseYAML(const std::string &yamlFile);

private:
	OptionParser(const std::string &name, const std::string &description)
	    : OptionGroup{name, description, nullptr} {}
};

} // namespace options
} // namespace fort
