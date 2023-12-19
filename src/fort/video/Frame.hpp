#pragma once

#include "Types.hpp"
#include <functional>
#include <memory>
#include <tuple>

namespace fort {
namespace video {

struct Frame {
	~Frame();
	uint8_t	         *Planes[4];
	int                  Linesize[4];
	size_t               Index;
	Duration             PTS;
	std::tuple<int, int> Size;
};

} // namespace video
} // namespace fort
