#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>

extern "C" {
#include <libavutil/imgutils.h>
}

#include "details/AVCall.hpp"

#include <fort/utils/Defer.hpp>

namespace fort {
namespace video {

class ReaderTest : public ::testing::Test {
protected:
	static std::filesystem::path TempDir;
	const static int             WIDTH  = 40;
	const static int             HEIGHT = 30;

	static void SetUpTestSuite() {
		std::string tmpDirtemplate = std::filesystem::temp_directory_path() /
		                             "fort-video-reader-tests-XXXXXX";
		TempDir = mkdtemp(const_cast<char *>(tmpDirtemplate.c_str()));

		std::array<uint8_t, 3 * WIDTH * HEIGHT> frame;
		{
			std::ofstream file(TempDir / "video.raw", std::ios_base::binary);

			if (!file) {
				throw std::runtime_error("could not open rawvideo file");
			}
			for (int i = 0; i < 255; i++) {
				memset(&frame[0], i, 3 * WIDTH * HEIGHT);
				file.write((const char *)(&frame[0]), 3 * WIDTH * HEIGHT);
			}
		}

		std::ostringstream cmd;
		cmd << "ffmpeg -f rawvideo -hide_banner -loglevel error -pix_fmt "
		    << av_get_pix_fmt_name(AV_PIX_FMT_BGR24) << " -video_size " << WIDTH
		    << "x" << HEIGHT << " -framerate 24 -i "
		    << (TempDir / "video.raw").string()
		    << " -c:v libx264 -preset fast -crf 22 "
		    << (TempDir / "video.mp4").string();

		int res = std::system(cmd.str().c_str());
		if (res != 0) {
			throw std::runtime_error("could not re-encode video in mp4");
		}
	}

	static void TearDownTestSuite() {
		std::filesystem::remove_all(TempDir);
	}
};

std::filesystem::path ReaderTest::TempDir;

TEST_F(ReaderTest, Fail) {
	ADD_FAILURE() << "Implement me";
}

} // namespace video
} // namespace fort
