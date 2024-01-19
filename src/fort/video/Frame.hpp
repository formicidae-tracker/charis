#pragma once

#include "Types.hpp"
#include <functional>
#include <memory>
#include <tuple>

namespace fort {
namespace video {

struct Frame {
	Frame(int width, int height, PixelFormat format, int alignement = 32);
	Frame(
	    const Resolution &resolution, PixelFormat format, int alignement = 32
	);

	~Frame();

	Frame(const Frame &other)            = delete;
	Frame &operator=(const Frame &other) = delete;

	uint8_t    *Planes[4];
	int         Linesize[4];
	PixelFormat Format;
	size_t      Index;
	Duration    PTS;
	Resolution  Size;
};

} // namespace video
} // namespace fort
