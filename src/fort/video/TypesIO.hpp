#pragma once

#include "Types.hpp"
#include <iostream>
#include <string>

extern "C" {
#include <libavutil/pixdesc.h>
}

namespace std {
inline string to_string(const fort::video::Resolution &size) {
	return std::to_string(size.Width) + "x" + std::to_string(size.Height);
}

inline string to_string(fort::video::PixelFormat format) {
	return av_get_pix_fmt_name(format);
}
} // namespace std

inline std::ostream &
operator<<(std::ostream &out, const fort::video::Resolution &size) {
	return out << size.Width << "x" << size.Height;
}

inline std::ostream &
operator<<(std::ostream &out, const fort::video::PixelFormat format) {
	return out << av_get_pix_fmt_name(format);
}
