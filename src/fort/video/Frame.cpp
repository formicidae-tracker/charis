#include "Frame.hpp"
#include "details/AVCall.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

namespace fort {
namespace video {

Frame::Frame(int width, int height, PixelFormat format, int alignement)
    : Format{format}
    , Size{width, height} {

	details::AVCall(
	    av_image_alloc,
	    Planes,
	    Linesize,
	    width,
	    height,
	    format,
	    alignement
	);
}

Frame::Frame(const Resolution &size, PixelFormat format, int alignement)
    : Format{format}
    , Size{size} {

	details::AVCall(
	    av_image_alloc,
	    Planes,
	    Linesize,
	    size.Width,
	    size.Height,
	    format,
	    alignement
	);
}

Frame::~Frame() {
	av_freep(Planes);
}

} // namespace video
} // namespace fort
