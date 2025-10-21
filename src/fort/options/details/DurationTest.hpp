#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>

#include <fort/utils/Defer.hpp>
#include <regex>

namespace fort {
namespace options {
namespace details {
using Duration = std::chrono::duration<int64_t, std::nano>;
}
} // namespace options
} // namespace fort

constexpr int64_t SECOND = 1000000000L;

static inline std::ostream &
operator<<(std::ostream &out, const fort::options::details::Duration &d) {
	using namespace fort::options::details;
	auto flags = out.flags();
	defer {
		out.flags(flags);
	};
	auto abs = std::abs(d.count());
	if (abs < 1000L) {
		return out << d.count() << "ns";
	}
	if (abs < 1000000L) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1000.0)
		           << "us";
	}

	if (abs < SECOND) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1.0e6)
		           << "ms";
	}

	if (abs < 60U * SECOND) {
		return out << std::fixed << std::setprecision(3) << (d.count() / 1.0e9)
		           << "s";
	}

	Duration seconds = Duration{d.count() % (60L * SECOND)};
	int64_t  minutes = (d.count() - seconds.count()) / 60L * SECOND;
	if (std::abs(minutes) < 60L) {
		return out << minutes << "m" << seconds;
	}

	int64_t hours = minutes / 60L;
	minutes -= 60L * hours;
	return out << hours << "h" << minutes << "m" << seconds;
}

std::istream &
operator>>(std::istream &in, fort::options::details::Duration &value) {
	using namespace fort::options::details;
	static std::regex durationRx{
	    "(-)?([0-9]+h)?([0-9]+m)?([0-9\\.eE+-]+)(ns|us|ms|s)",
	};
	std::string tmp;
	in >> tmp;
	std::smatch m;
	if (std::regex_match(tmp, m, durationRx) == false || m.size() != 6) {
		in.setstate(std::ios_base::failbit);
		return in;
	}

	int64_t sign = m[1] == "-" ? -1 : 1;
	int64_t hours =
	    m[2] == "" ? 0
	               : std::stoll(std::string(m[2]).substr(0, m[2].length() - 1));
	int64_t minutes =
	    m[3] == "" ? 0
	               : std::stoll(std::string(m[3]).substr(0, m[3].length() - 1));

	std::string units = m[5];

	if ((hours != 0 || minutes != 0) && units != "s") {
		in.setstate(std::ios_base::failbit);
		return in;
	}

	double rest = std::stod(m[4]);

	double unitF = 0.0;
	if (units == "s") {
		unitF = 1e9;
	} else if (units == "ms") {
		unitF = 1e6;
	} else if (units == "us") {
		unitF = 1e3;
	} else if (units == "ns") {
		unitF = 1.0;
	} else {
		in.setstate(std::ios_base::failbit);
		return in;
	}
	value = Duration{
	    sign * (hours * 3600L * SECOND + minutes * 60L * SECOND +
	            int64_t(rest * unitF))
	};

	return in;
}
