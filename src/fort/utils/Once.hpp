#pragma once

#include <utility>

#define fu_ONCE_UNIQUE_NAME_INNER(a, b) a##b
#define fu_ONCE_UNIQUE_NAME(base, line) fu_ONCE_UNIQUE_NAME_INNER(base, line)
#define fu_ONCE_NAME                    fu_ONCE_UNIQUE_NAME(zz_once, __LINE__)
#define once                                                                   \
	static auto fu_ONCE_NAME = fort::utils::details::Oncer_void{} *[&]()

namespace fort {
namespace utils {
namespace details {
template <typename Lambda> struct Oncer {
	Oncer(Lambda &&lambda) {
		std::forward<Lambda>(lambda)();
	}
};

struct Oncer_void {};

template <typename Lambda>
Oncer<Lambda> operator*(Oncer_void, Lambda &&lambda) {
	return {std::forward<Lambda>(lambda)};
}

} // namespace details
} // namespace utils
} // namespace fort
