#include "fort/video/Reader.hpp"
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <tuple>

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
	constexpr static int         WIDTH      = 40;
	constexpr static int         HEIGHT     = 30;
	constexpr static Resolution  RESOLUTION = {WIDTH, HEIGHT};
	constexpr static int         LENGTH     = 255;

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
			for (int i = 0; i < LENGTH; i++) {
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

TEST_F(ReaderTest, CanGetBaseInformations) {
	Reader r{TempDir / "video.mp4"};
	EXPECT_EQ(r.Length(), LENGTH);
	EXPECT_EQ(r.Duration().count(), int64_t(LENGTH * 1e9) / 24);
	EXPECT_NEAR(r.AverageFrameDuration().count(), int64_t(1e9) / 24, 1);
	EXPECT_EQ(r.Size(), RESOLUTION);
	EXPECT_EQ(r.Position(), 0);
}

TEST_F(ReaderTest, CanGrabAllFrames) {
	Reader r{TempDir / "video.mp4"};
	auto   frame = r.CreateFrame();
	for (size_t i = 0; i < 255; i++) {
		SCOPED_TRACE("frame: " + std::to_string(i));
		EXPECT_EQ(r.Position(), i);
		EXPECT_NO_THROW({ EXPECT_TRUE(r.Read(*frame)); });
		EXPECT_EQ(frame->Index, i);
		EXPECT_NEAR(frame->PTS.count(), int64_t(i * 1e9) / 24, 1);
		EXPECT_NEAR(frame->Planes[0][0], i, 1);
		EXPECT_NEAR(frame->Planes[0][1], i, 1);
		EXPECT_NEAR(frame->Planes[0][2], i, 1);
	}
	EXPECT_EQ(r.Position(), r.Length());
}

TEST_F(ReaderTest, CanSeekForward) {
	Reader r{TempDir / "video.mp4"};

	EXPECT_NO_THROW({ r.SeekFrame(127); });
	EXPECT_EQ(r.Position(), 127);
	auto frame = r.CreateFrame();
	ASSERT_TRUE(r.Read(*frame));
	EXPECT_EQ(frame->Index, 127);
	EXPECT_EQ(frame->Planes[0][0], 127);
	EXPECT_EQ(frame->Planes[0][1], 127);
	EXPECT_EQ(frame->Planes[0][2], 127);
}

TEST_F(ReaderTest, CanEfficientlySeekForward) {
	Reader r{TempDir / "video.mp4"};

	EXPECT_NO_THROW({ r.SeekFrame(127, false); });
	EXPECT_GT(r.Position(), 100);
}

TEST_F(ReaderTest, CanSeekBackward) {
	Reader r{TempDir / "video.mp4"};

	for (size_t i = 0; i < 127; i++) {
		r.Grab();
	}

	EXPECT_NO_THROW({ r.SeekFrame(64); });
	EXPECT_EQ(r.Position(), 64);
	auto frame = r.CreateFrame();
	ASSERT_TRUE(r.Read(*frame));
	EXPECT_EQ(frame->Index, 64);
	EXPECT_EQ(frame->Planes[0][0], 64);
	EXPECT_EQ(frame->Planes[0][1], 64);
	EXPECT_EQ(frame->Planes[0][2], 64);
}

TEST_F(ReaderTest, CanSeekBackwardAfterReachingEnd) {
	Reader r{TempDir / "video.mp4"};

	for (size_t i = 0; i < 255; i++) {
		r.Grab();
	}
	ASSERT_FALSE(r.Grab());

	EXPECT_NO_THROW({ r.SeekFrame(64); });
	EXPECT_EQ(r.Position(), 64);
	auto frame = r.CreateFrame();
	ASSERT_TRUE(r.Read(*frame));
	EXPECT_EQ(frame->Index, 64);
	EXPECT_EQ(frame->Planes[0][0], 64);
	EXPECT_EQ(frame->Planes[0][1], 64);
	EXPECT_EQ(frame->Planes[0][2], 64);
}

TEST_F(ReaderTest, CanEfficientlySeekBackward) {
	Reader r{TempDir / "video.mp4"};
	for (size_t i = 0; i < 255; i++) {
		r.Grab();
	}

	EXPECT_NO_THROW({ r.SeekFrame(64, false); });
	EXPECT_GT(r.Position(), 30);
}

TEST_F(ReaderTest, CanReadOneEveryTwo) {
	Reader r{TempDir / "video.mp4"};

	auto frame = r.CreateFrame();
	for (size_t i = 0; i < 255; i++) {
		SCOPED_TRACE(std::to_string(i));
		if (i % 2 == 0) {
			EXPECT_TRUE(r.Grab());
		} else {
			EXPECT_EQ(r.Position(), i - 1);
			EXPECT_TRUE(r.Grab());
			EXPECT_EQ(r.Position(), i);
			EXPECT_TRUE(r.Read(*frame));
			EXPECT_EQ(frame->Index, i);
			EXPECT_NEAR(frame->Planes[0][0], i, 1);
		}
	}
}

} // namespace video
} // namespace fort
