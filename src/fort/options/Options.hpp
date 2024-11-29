#pragma once

#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <fort/options/details/ArgsLexer.hpp>
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
	    , d_flags{ValuesByShort{}}
	    , d_help{std::make_shared<details::Option<bool>>(details::OptionArgs{
	          .Flag        = 'h',
	          .Name        = "help",
	          .Description = "displays this help message",
	      })} {
		std::cerr << "Created group " << this << std::endl;
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
		pushOption(opt);
		return *opt;
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
		    *static_cast<T *>(d_subgroups.insert({name, std::make_unique<T>()})
		                          .first->second.get());
		for (const auto &[flag, opt] : res.d_flags.value()) {
			if (opt->Name.size() != 0) {
				throw std::invalid_argument{
				    "subgroup '" + name + "' option '" + opt->Name +
				    " cannot have short flag '" + std::string(1, flag) + "'"};
			}
			opt->Flag = 0;
			opt->Name = std::string(1, flag);
			res.d_options.insert({opt->Name, opt});
		}

		res.d_flags       = std::nullopt;
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
		if (name.size() == 1 && d_flags.has_value()) {
			auto opt = d_flags.value().find(name[0]);
			if (opt == d_flags.value().end()) {
				throw std::out_of_range{"unknown short flag '" + name + "'"};
			}
			opt->second->Parse(value);
			return;
		}

		auto pos = name.find('.');
		if (pos != std::string::npos) {
			auto subgroupName = name.substr(0, pos);
			auto subgroup     = d_subgroups.find(subgroupName);
			if (subgroup == d_subgroups.end()) {
				throw std::out_of_range{
				    "unknown option '" + name + "': unknown subgroup '" +
				    subgroupName + "'"};
			}
			subgroup->second->Set(name.substr(pos + 1), value);
			return;
		}

		auto opt = d_options.find(name);
		if (opt == d_options.end()) {
			throw std::out_of_range{"unknown option '" + name + "'"};
		}
		opt->second->Parse(value);
	}

	void SetDescription(const std::string &description) {
		if (d_parent.has_value()) {
			throw std::logic_error{
			    "description could only be set on the root group"};
		}
		d_description = description;
	}

	void SetName(const std::string &name) {
		if (d_parent.has_value()) {
			throw std::logic_error{"name could only be set on the root group"};
		}
		d_name = name;
	}

	void ParseArguments(int &argc, const char **argv) {
		if (d_parent.has_value()) {
			throw std::logic_error{"Only root group can parse arguments"};
		}
		mayAddHelp();
		auto tokens = details::lexArguments(argc, (char **)argv);

		if (argc == 0 || tokens[0].Type != details::TokenType::VALUE) {
			throw std::runtime_error{"program name missing"};
		}

		d_name = tokens[0].Value;
		argc   = 1;

		for (int i = 1; i < tokens.size(); ++i) {
			if (tokens[i].Type == details::TokenType::VALUE) {
				// unconsummed value, simply add it to the argument.
				argv[argc] = argv[i];
				++argc;
				continue;
			}

			if (tokens[i].Value == "h" || tokens[i].Value == "help") {
				FormatUsage(std::cerr);
				exit(0);
			}

			OptionPtr opt = findOption(tokens[i].Value);
			if (opt == nullptr) {
				throw std::runtime_error{
				    "unknown flag '" + tokens[i].Value + "'"};
			}

			if (opt->NumArgs == 0 &&
			    tokens[i].Type != details::TokenType::IDENTIFIER_WITH_VALUE) {
				opt->Parse(std::nullopt);
			} else if (i + 1 == tokens.size() || tokens[i + 1].Type != details::TokenType::VALUE) {
				throw std::runtime_error{"missing required value after flag"};
			}
			opt->Parse(tokens[i + 1].Value);
			i += 1; // we consumed two token here
		}
	}

	void FormatUsage(std::ostream &out, const std::string &nm = "") const {
		if (d_parent.has_value() == false) {
			out << "Usage:" << std::endl
			    << d_name << " [OPTIONS]" << std::endl
			    << std::endl;
			if (d_description.empty() == false) {
				out << d_description << std::endl << std::endl;
			}
			out << "Application Options:" << std::endl;
		} else {
			out << d_name << ":";
			if (d_description.empty() == false) {
				out << " " << d_description;
			}
			out << std::endl;
		}

		for (const auto &opt : d_inOrder) {
			formatOption(out, opt, nm);
		}

		for (const auto &[name, subgroup] : d_subgroups) {
			out << std::endl;
			subgroup->FormatUsage(out, nm.empty() ? name : nm + "." + name);
		}

		if (d_parent.has_value() == false) {
			out << std::endl << "Help Options:" << std::endl;
			formatOption(out, d_help, "");
		}
	}

	void ParseYAML(const std::string &filename) {
		throw std::runtime_error("not yet implemented");
	}

	void ParseINI(const std::string &filename) {
		throw std::runtime_error("not yet implemented");
	}

private:
	constexpr static int COLUMN_SIZE = 20;
	using OptionPtr                  = std::shared_ptr<details::OptionBase>;
	using GroupPtr                   = std::unique_ptr<Group>;

	using ValuesByShort = std::map<char, OptionPtr>;
	using ValuesByLong  = std::map<std::string, OptionPtr>;

	void formatOption(
	    std::ostream &out, const OptionPtr &opt, const std::string &nm
	) const {
		out << "  ";
		if (opt->Flag == 0) {
			out << "    ";
		} else {
			out << "-" << opt->Flag << (opt->Name.empty() ? "  " : ", ");
		}

		if (opt->Name.empty()) {
			out << std::string(COLUMN_SIZE - 6, ' ');
		} else if (nm.empty()) {
			size_t spaceSize =
			    std::max(0, COLUMN_SIZE - 8 - int(opt->Name.size()));
			out << "--" << opt->Name << std::string(spaceSize, ' ');
		} else {
			size_t spaceSize = std::max(
			    0,
			    COLUMN_SIZE - 9 - int(opt->Name.size()) - int(nm.size())
			);
			out << "--" << nm << "." << opt->Name
			    << std::string(spaceSize, ' ');
		}

		if (opt->Description.empty()) {
			return;
		}
		out << ": " << opt->Description;
		if (opt->Required) {
			out << " [required]";
		} else if (opt->NumArgs != 0) {
			out << " [default: ";
			opt->Format(out);
			out << "]";
		}
		out << std::endl;
	}

	OptionPtr findOption(const std::string &name) {
		if (name.size() == 1 && d_flags.has_value()) {
			auto iter = d_flags.value().find(name[0]);
			if (iter == d_flags.value().end()) {
				return nullptr;
			}
			return iter->second;
		}
		auto iter = d_options.find(name);
		if (iter == d_options.end()) {
			return nullptr;
		}
		return iter->second;
	}

	details::OptionArgs checkArgs(
	    const std::string &designator, const std::string &description
	) const {
		if (description.empty()) {
			throw std::invalid_argument{"Description cannot be empty"};
		}

		const auto [flag, name] = details::parseDesignators(designator);

		if (d_options.count(name) > 0) {
			throw std::invalid_argument{
			    "option '" + name + "' already specified",
			};
		}

		if (d_flags.has_value() == false) {
			if (flag != 0) {
				throw std::runtime_error{"cannot set short flag in subgroup"};
			}
		} else if (flag != 0) {
			auto sh = d_flags.value().find(flag);
			if (sh != d_flags.value().end()) {
				throw std::invalid_argument{
				    "short flag '" + std::string(1, flag) +
				        "' already used by option '" + sh->second->Name + "'",
				};
			}
		}

		if (flag == 'h' || name == "help") {
			throw std::invalid_argument{"-h/--help flag is reserved"};
		}

		return {
		    .Flag        = flag,
		    .Name        = name,
		    .Description = description,
		};
	}

	void mayAddHelp() {
		if (d_parent.has_value() || d_options.count("help") > 0) {
			return;
		}
	}

	void pushOption(OptionPtr option) {
		if (option->Name.size() > 0) {
			std::cerr << "adding option '" << option->Name << "'" << std::endl;
			d_options[option->Name] = option;
		}
		if (option->Flag != 0) {
			d_flags.value().insert({option->Flag, option});
		}
		d_inOrder.push_back(option);
	}

	std::string d_name;
	std::string d_description;

	ValuesByLong d_options;

	std::optional<Group *>          d_parent;
	std::optional<ValuesByShort>    d_flags;
	std::map<std::string, GroupPtr> d_subgroups;

	std::vector<OptionPtr> d_inOrder;
	OptionPtr              d_help;
};

} // namespace options
} // namespace fort
