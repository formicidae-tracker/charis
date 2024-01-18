#include <charconv>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fort/video/Frame.hpp>
#include <fort/video/PNG.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <libavutil/log.h>
#include <memory>
extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
}
#include <stdexcept>

namespace fort {
namespace video {
class PNGTest : public ::testing::Test {
protected:
	static std::filesystem::path TempDir;

	constexpr static int WIDTH  = 32;
	constexpr static int HEIGHT = 8;
	constexpr static int SIZE   = WIDTH * HEIGHT;

	static std::array<uint8_t, SIZE>     Gray;
	static std::array<uint8_t, 3 * SIZE> RGB;

	static void SetUpTestSuite() {
		std::string dirTemplate = std::filesystem::temp_directory_path() /
		                          "fort-video-png-tests-XXXXXX";
		TempDir = mkdtemp(const_cast<char *>(dirTemplate.c_str()));

		WriteRGBImage();
		WriteGrayImage();
	}

	static void WriteRGBImage() {
		for (int i = 0; i < SIZE; i++) {
			RGB[3 * i + 0] = i;
			RGB[3 * i + 1] = (85 + i) % 256;
			RGB[3 * i + 2] = (170 + i) % 256;
		}

		{
			std::ofstream file{
			    TempDir / "video-rgb.raw",
			    std::ios_base::binary};
			if (!file) {
				throw std::runtime_error{"could not open rawvideo file"};
			}
			file.write((const char *)(RGB.data()), 3 * SIZE);
		}

		std::ostringstream cmd;
		cmd << "ffmpeg -f rawvideo -hide_banner -loglevel error -pix_fmt "
		    << av_get_pix_fmt_name(AV_PIX_FMT_RGB24) << " -video_size " << WIDTH
		    << "x" << HEIGHT << " -framerate 24 -i "
		    << (TempDir / "video-rgb.raw").string()
		    << " -vf \"select=eq(n\\,0)\" " << (TempDir / "rgb.png").string();

		int res = std::system(cmd.str().c_str());
		if (res != 0) {
			throw std::runtime_error("could not write PNG file");
		}
	}

	static void WriteGrayImage() {
		for (int i = 0; i < SIZE; i++) {
			Gray[i] = i;
		}

		{
			std::ofstream file{
			    TempDir / "video-gray.raw",
			    std::ios_base::binary};
			if (!file) {
				throw std::runtime_error{"could not open rawvideo file"};
			}
			file.write((const char *)(Gray.data()), SIZE);
		}

		std::ostringstream cmd;
		cmd << "ffmpeg -f rawvideo -hide_banner -loglevel error -pix_fmt "
		    << av_get_pix_fmt_name(AV_PIX_FMT_GRAY8) << " -video_size " << WIDTH
		    << "x" << HEIGHT << " -framerate 24 -i "
		    << (TempDir / "video-gray.raw").string()
		    << " -vf \"select=eq(n\\,0)\" " << (TempDir / "gray.png").string();

		int res = std::system(cmd.str().c_str());
		if (res != 0) {
			throw std::runtime_error("could not write PNG file");
		}
	}

	static void TearDownTestSuite() {
		if (HasFailure()) {
			return;
		}
		std::filesystem::remove_all(TempDir);
	}
};

std::filesystem::path               PNGTest::TempDir;
std::array<uint8_t, PNGTest::SIZE>     PNGTest::Gray;
std::array<uint8_t, 3 * PNGTest::SIZE> PNGTest::RGB;

TEST_F(PNGTest, CanReadGrayImage) {
	std::unique_ptr<video::Frame> gray;

	EXPECT_NO_THROW({ gray = ReadPNG(TempDir / "gray.png"); });

	ASSERT_EQ(gray->Size, std::make_tuple(WIDTH, HEIGHT));

	for (int i = 0; i < SIZE; i++) {
		SCOPED_TRACE(std::to_string(i));
		EXPECT_EQ(gray->Planes[0][i], Gray[i]);
	}
}

TEST_F(PNGTest, CanReadRGBImage) {
	std::unique_ptr<video::Frame> rgb;

	EXPECT_NO_THROW({ rgb = ReadPNG(TempDir / "rgb.png", AV_PIX_FMT_RGB24); });

	ASSERT_EQ(rgb->Size, std::make_tuple(WIDTH, HEIGHT));
	ASSERT_EQ(rgb->Format, AV_PIX_FMT_RGB24);

	for (int i = 0; i < SIZE; i++) {
		SCOPED_TRACE(std::to_string(i));
		EXPECT_EQ(rgb->Planes[0][3 * i + 0], RGB[3 * i + 0]);
		EXPECT_EQ(rgb->Planes[0][3 * i + 1], RGB[3 * i + 1]);
		EXPECT_EQ(rgb->Planes[0][3 * i + 2], RGB[3 * i + 2]);
	}
}

TEST_F(PNGTest, CanWriteGrayImage) {
	auto gray =
	    std::make_unique<video::Frame>(WIDTH, HEIGHT, AV_PIX_FMT_GRAY8, 4);

	memcpy(gray->Planes[0], Gray.data(), SIZE);

	auto path = TempDir / "gray-w.png";

	EXPECT_NO_THROW(WritePNG(path, *gray));

	std::unique_ptr<video::Frame> res;
	EXPECT_NO_THROW({ res = ReadPNG(path); });
	ASSERT_EQ(res->Format, AV_PIX_FMT_GRAY8);
	ASSERT_EQ(res->Size, std::make_tuple(WIDTH, HEIGHT));
	for (int i = 0; i < SIZE; i++) {
		SCOPED_TRACE(std::to_string(i));
		EXPECT_EQ(res->Planes[0][i], Gray[i]);
	}
}

TEST_F(PNGTest, CanWriteRGBImage) {
	auto rgb =
	    std::make_unique<video::Frame>(WIDTH, HEIGHT, AV_PIX_FMT_RGB24, 4);

	memcpy(rgb->Planes[0], RGB.data(), 3 * SIZE);
	auto path = TempDir / "rgb-w.png";
	EXPECT_NO_THROW(WritePNG(path, *rgb));

	std::unique_ptr<video::Frame> res;
	EXPECT_NO_THROW({ res = ReadPNG(path, AV_PIX_FMT_RGB24); });
	ASSERT_EQ(res->Format, AV_PIX_FMT_RGB24);
	ASSERT_EQ(res->Size, std::make_tuple(WIDTH, HEIGHT));

	for (int i = 0; i < SIZE; i++) {
		SCOPED_TRACE(std::to_string(i));
		EXPECT_EQ(res->Planes[0][3 * i + 0], RGB[3 * i + 0]);
		EXPECT_EQ(res->Planes[0][3 * i + 1], RGB[3 * i + 1]);
		EXPECT_EQ(res->Planes[0][3 * i + 2], RGB[3 * i + 2]);
	}
}

} // namespace video
} // namespace fort
