#pragma once

#include <chrono>
#include <ratio>
extern "C" {
#include <libavutil/pixfmt.h>
}

namespace fort {
namespace video {

using Duration = std::chrono::duration<int64_t, std::nano>;

using PixelFormat = AVPixelFormat;

} // namespace video
} // namespace fort
