#pragma once

#include <chrono>
#include <ratio>
#include <type_traits>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_id.h>
#include <libavutil/pixfmt.h>
}

namespace fort {
namespace video {

using Duration = std::chrono::duration<int64_t, std::nano>;

using PixelFormat = AVPixelFormat;

using CodecID = AVCodecID;

template <typename T, std::enable_if_t<std::is_integral_v<T>> * = nullptr>
struct Ratio {
	T Num;
	T Den;
};

} // namespace video
} // namespace fort
