#include "Frame.hpp"

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
