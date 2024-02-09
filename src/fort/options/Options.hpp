#pragma once

#include <fort/options/details/Option.hpp>
#include <fort/options/details/Traits.hpp>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace fort {
namespace options {
constexpr char NO_SHORT = 0;

using OptionArgs = details::OptionArgs;

class OptionGroup : std::enable_shared_from_this<OptionGroup> {
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
			d_parent     = std::nullopt;
			d_shortFlags = ValuesByShort{};
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

	void checkArgs(const OptionArgs &args) {
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
};

class OptionParser : public OptionGroup {};

} // namespace options
} // namespace fort
