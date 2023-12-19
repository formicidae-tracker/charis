#pragma once

#include <cpptrace/cpptrace.hpp>
#include <utility>
extern "C" {
#include <libavutil/error.h>
}

namespace fort {
namespace video {

namespace details {

class AVError : cpptrace::runtime_error {
public:
	template <typename Function>
	AVError(int error, Function &&fn) noexcept
	    : cpptrace::runtime_error{Reason(error, std::forward<Function>(fn))}
	    , d_error{error} {}

	template <typename Function>
	static std::string Reason(int error, Function &&fn) noexcept {
		char buffer[1024];
		av_strerror(error, buffer, 1024);
		return cpptrace::demangle(typeid(Function).name()) + " error (" +
		       std::to_string(error) + "): " + buffer;
	}

	int Code() const noexcept {
		return d_error;
	}

private:
	int d_error;
};

template <typename Function, typename... Args>
int AVCall(Function &&fn, Args &&...args) {
	int res = std::forward<Function>(fn)(std::forward<Args>(args)...);
	if (res < 0) {
		throw AVError(res, std::forward<Function>(fn));
	}
	return res;
}

} // namespace details
} // namespace video
} // namespace fort
