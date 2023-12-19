#include "Frame.hpp"
#include <libavutil/mem.h>

extern "C" {
#include <libavutil/avutil.h>
}

namespace fort {
namespace video {
Frame::~Frame() {
	av_freep(Planes);
}

} // namespace video
} // namespace fort
