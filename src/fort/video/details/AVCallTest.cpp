#include "gtest/gtest.h"
#include <fort/video/details/AVCall.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

namespace fort {
namespace video {
namespace details {

class AVCallUTest : public ::testing::Test {};

TEST_F(AVCallUTest, ErrorFormatting) {
	EXPECT_STREQ(
	    AVError(AVERROR(EAGAIN), avcodec_receive_frame).message(),
	    "avcodec_receive_frame() error (-11): Resource temporarily unavailable"
	);
}
} // namespace details
} // namespace video
} // namespace fort
