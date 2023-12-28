#include <filesystem>
#include <fort/video/Encoder.hpp>
#include <fort/video/Frame.hpp>
#include <fort/video/Reader.hpp>
#include <fort/video/Writer.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <libavutil/pixfmt.h>
#include <string>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace fort {
namespace video {

class WriterTest : public ::testing::Test {
protected:
	static std::filesystem::path TempDir;
	constexpr static int         WIDTH              = 40;
	constexpr static int         HEIGHT             = 30;
	constexpr static int         FULL_PLANE_SIZE    = WIDTH * HEIGHT;
	constexpr static int         QUARTER_PLANE_SIZE = (WIDTH * HEIGHT) / 4;
	constexpr static int IMAGE_SIZE = FULL_PLANE_SIZE + 2 * QUARTER_PLANE_SIZE;

	using ImageData = std::array<uint8_t, IMAGE_SIZE>;
	static std::vector<ImageData> Frames;

	static void SetUpTestSuite() {
		std::string tmpDirTemplate = std::filesystem::temp_directory_path() /
		                             "fort-video-writer-tests-XXXXXX";
		TempDir = mkdtemp(const_cast<char *>(tmpDirTemplate.c_str()));

		Frames.resize(255);
		{
			std::ofstream file(TempDir / "video.raw", std::ios_base::binary);

			if (!file) {
				throw std::runtime_error("could not open video.raw file");
			}
			for (int i = 0; i < 255; i++) {

				FillYuv420p(Frames[i], i);
				file.write((const char *)(&Frames[i][0]), IMAGE_SIZE);
			}
		}

		std::ostringstream cmd;
		cmd << "ffmpeg -f rawvideo -hide_banner -loglevel error -pix_fmt "
		    << av_get_pix_fmt_name(AV_PIX_FMT_YUV420P) << " -video_size "
		    << WIDTH << "x" << HEIGHT << " -framerate 24 -i "
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

	static void FillYuv420p(ImageData &data, int index) {
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; ++x) {
				data[y * WIDTH + x] = (x + y + index * 3) % 255;
			}
		}
		for (int y = 0; y < HEIGHT / 2; y++) {
			for (int x = 0; x < WIDTH / 2; ++x) {
				data[y * WIDTH / 2 + x + FULL_PLANE_SIZE] =
				    (128 + y + index * 2) % 255;
				data[y * WIDTH / 2 + x + FULL_PLANE_SIZE + QUARTER_PLANE_SIZE] =
				    (64 + x + index * 5) % 255;
			}
		}
	}
};

std::filesystem::path WriterTest::TempDir;
std::vector<WriterTest::ImageData> WriterTest::Frames;

TEST_F(WriterTest, CanEncodeGray8) {
	auto path = TempDir / "generated-gray.mp4";
	{
		Writer w{
		    Writer::Params{.Path = path},
		    Encoder::Params{
		        .Size{40, 30},
		        .Framerate = {24, 1},
		        .Format    = AV_PIX_FMT_GRAY8,
		    }};

		Frame frame{40, 30, AV_PIX_FMT_GRAY8, 16};
		for (int i = 0; i < 255; i++) {
			memset(frame.Planes[0], i, 40 * 30);
			w.Write(frame);
		}
	}

	Reader r{path};
	EXPECT_EQ(r.Size(), std::make_tuple(40, 30));

	for (size_t i = 0; i < 255; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto f = r.Grab();
		EXPECT_TRUE(f);
		if (f) {
			EXPECT_NEAR(f->Planes[0][0], i, 1);
		}
	}
}

TEST_F(WriterTest, CanEncodeYUV) {
	auto path = TempDir / "generated-yuv.mp4";
	{
		Writer w{
		    Writer::Params{.Path = path},
		    Encoder::Params{
		        .Size{40, 30},
		        .Framerate = {24, 1},
		        .Format    = AV_PIX_FMT_YUV420P,
		    }};
		Frame frame{40, 30, AV_PIX_FMT_YUV420P};

		for (const auto &data : Frames) {
			const uint8_t *planes[4] = {
			    &(data[0]),
			    &(data[FULL_PLANE_SIZE]),
			    &(data[FULL_PLANE_SIZE + QUARTER_PLANE_SIZE]),
			    nullptr,
			};
			int linesizes[4] = {WIDTH, WIDTH / 2, WIDTH / 2, 0};

			av_image_copy(
			    frame.Planes,
			    frame.Linesize,
			    planes,
			    linesizes,
			    AV_PIX_FMT_YUV420P,
			    WIDTH,
			    HEIGHT
			);

			w.Write(frame);
		}
	}

	Reader r{path, AV_PIX_FMT_YUV420P};
	ASSERT_EQ(r.Size(), std::make_tuple(WIDTH, HEIGHT));

	for (int i = 0; i < 255; i++) {
		SCOPED_TRACE(std::to_string(i));
		auto f = r.Grab();
		EXPECT_TRUE(f);
		if (f) {
			EXPECT_NEAR(f->Planes[0][0], Frames[i][0], 1);
		}
	}
}

} // namespace video
} // namespace fort
