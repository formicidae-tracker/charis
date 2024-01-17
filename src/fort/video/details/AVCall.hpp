#pragma once

#include <cpptrace/cpptrace.hpp>
#include <fort/video/details/FunctionName.hpp>
#include <utility>
extern "C" {
#include <libavutil/error.h>
}

namespace fort {
namespace video {

namespace details {

class AVError : public cpptrace::runtime_error {
public:
	template <typename Function>
	AVError(int error, Function &&fn) noexcept
	    : cpptrace::runtime_error{Reason(error, std::forward<Function>(fn))}
	    , d_error{error} {}

	template <typename Function>
	static std::string Reason(int error, Function &&fn) noexcept {
		char buffer[1024];
		av_strerror(error, buffer, 1024);
		return FunctionName(std::forward<Function>(fn)) + "() error (" +
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

template <typename Object, typename Function, typename... Args>
Object *AVAlloc(Function &&fn, Args &&...args) {
	Object *res = std::forward<Function>(fn)(std::forward<Args>(args)...);
	if (res == nullptr) {
		throw AVError(AVERROR(ENOMEM), std::forward<Function>(fn));
	}
	return res;
}

} // namespace details
} // namespace video
} // namespace fort
