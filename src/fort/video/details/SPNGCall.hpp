#pragma once

#include <fort/video/details/FunctionName.hpp>
#include <spng.h>

namespace fort {
namespace video {
namespace details {

class SPNGError : public cpptrace::runtime_error {
public:
	template <typename Function>
	SPNGError(int error, Function && fn)
		: cpptrace::runtime_error{
				FunctionName(fn) +
		    "() error (" + std::to_string(error) +
		    "): " + spng_strerror(error)}
		, d_code{error} {
	}

	int Code() const noexcept {
		return d_code;
	}

private:
	int d_code;
};

template <typename Function, typename... Args>
void SPNGCall(Function &&fn, Args &&...args) {
	int err = std::forward<Function>(fn)(std::forward<Args>(args)...);
	if (err != 0) {
		throw SPNGError(err, fn);
	}
}

} // namespace details
} // namespace video
} // namespace fort
