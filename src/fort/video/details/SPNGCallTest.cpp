#include <gtest/gtest.h>

#include "SPNGCall.hpp"
#include "spng.h"

namespace fort {
namespace video {
namespace details {
class SPNGCallUTest : public ::testing::Test {};

TEST_F(SPNGCallUTest, ErrorFormatting) {
	EXPECT_STREQ(
	    SPNGError(SPNG_IO_ERROR, spng_strerror).message(),
	    "spng_strerror() error (-2): stream error"
	);

	try {
		SPNGCall(spng_decode_image, nullptr, nullptr, 0, 0, 0);
		ADD_FAILURE() << "should throw an exception";

	} catch (const SPNGError &e) {
		EXPECT_STREQ(
		    e.message(),
		    "spng_decode_image() error (1): invalid argument"
		);
	}
}

} // namespace details
} // namespace video
} // namespace fort
