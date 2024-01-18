#include "PNG.hpp"
#include "spng.h"
#include <fort/utils/Defer.hpp>
#include <fort/video/Types.hpp>
#include <fort/video/details/SPNGCall.hpp>

#include <sstream>
#include <stdexcept>
#include <system_error>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
}

namespace fort {
namespace video {
namespace details {
std::tuple<int, uint8_t> MapFormat(PixelFormat fmt) {
	switch (fmt) {
	case AV_PIX_FMT_GRAY8:
		return {SPNG_FMT_G8, SPNG_COLOR_TYPE_GRAYSCALE};
	case AV_PIX_FMT_RGB24:
		return {SPNG_FMT_RGB8, SPNG_COLOR_TYPE_TRUECOLOR};
	default:
		throw std::invalid_argument{
		    std::string("Unsupported format ") + av_get_pix_fmt_name(fmt),
		};
	}
}
} // namespace details

std::unique_ptr<video::Frame>
ReadPNG(const std::filesystem::path &path, PixelFormat format) {
	auto [fmt, _] = details::MapFormat(format);

	FILE *file = std::fopen(path.c_str(), "rb");
	if (file == nullptr) {
		std::ostringstream oss;
		oss << "open(" << path << ", \"rb\")";
		throw std::system_error{errno, std::generic_category(), oss.str()};
	}
	auto ctx = spng_ctx_new(0); // create a decoder context
	defer {
		spng_ctx_free(ctx);
		fclose(file);
	};

	details::SPNGCall(spng_set_png_file, ctx, file);

	struct spng_ihdr ihdr;
	details::SPNGCall(spng_get_ihdr, ctx, &ihdr);

	size_t decodedSize;
	details::SPNGCall(spng_decoded_image_size, ctx, fmt, &decodedSize);

	constexpr static size_t MAX_IMAGE_SIZE = 8192 * 8192;

	if (decodedSize > MAX_IMAGE_SIZE) {

		std::ostringstream oss;
		oss << "PNG file " << path << " is too large: decoded size "
		    << decodedSize << " is larger than " << MAX_IMAGE_SIZE
		    << " declared width: " << ihdr.width
		    << " declared height: " << ihdr.height;
		throw cpptrace::runtime_error(oss.str());
	}

	auto res =
	    std::make_unique<video::Frame>(ihdr.width, ihdr.height, format, 32);

	details::SPNGCall(
	    spng_decode_image,
	    ctx,
	    nullptr,
	    0,
	    fmt,
	    SPNG_DECODE_PROGRESSIVE
	);

	int                  error;
	struct spng_row_info rowInfo;

	try {
		while (true) {
			details::SPNGCall(spng_get_row_info, ctx, &rowInfo);

			// note that video::Frame is not necessarly packed, use stride
			// instead of width.
			uint8_t *rowAddr =
			    res->Planes[0] + res->Linesize[0] * rowInfo.row_num;

			details::SPNGCall(spng_decode_row, ctx, rowAddr, res->Linesize[0]);
		}
	} catch (const details::SPNGError &e) {
		if (e.Code() != SPNG_EOI) {
			throw;
		}
	}

	return res;
}

void WritePNG(const std::filesystem::path &path, const video::Frame &frame) {
	auto [fmt, colorType] = details::MapFormat(frame.Format);

	auto file = fopen(path.string().c_str(), "wb");
	if (file == nullptr) {
		std::ostringstream oss;
		oss << "fopen(" << path << ", \"wb\") error";
		throw std::system_error{errno, std::generic_category(), oss.str()};
	}
	auto ctx = spng_ctx_new(SPNG_CTX_ENCODER);
	defer {
		spng_ctx_free(ctx);
		fclose(file);
	};

	details::SPNGCall(spng_set_png_file, ctx, file);

	struct spng_ihdr ihdr {
		.width  = uint32_t(std::get<0>(frame.Size)),
		.height = uint32_t(std::get<1>(frame.Size)), .bit_depth = 8,
		.color_type = colorType, .filter_method = SPNG_FILTER_NONE,
		.interlace_method = SPNG_INTERLACE_NONE,
	};

	details::SPNGCall(spng_set_ihdr, ctx, &ihdr);

	details::SPNGCall(
	    spng_encode_image,
	    ctx,
	    nullptr,
	    0,
	    SPNG_FMT_PNG,
	    SPNG_ENCODE_FINALIZE | SPNG_ENCODE_PROGRESSIVE
	);

	try {
		struct spng_row_info rowInfo;
		while (true) {
			details::SPNGCall(spng_get_row_info, ctx, &rowInfo);

			uint8_t *addr =
			    frame.Planes[0] + frame.Linesize[0] * rowInfo.row_num;

			details::SPNGCall(spng_encode_row, ctx, addr, frame.Linesize[0]);
		}
	} catch (const details::SPNGError &e) {
		if (e.Code() != SPNG_EOI) {
			throw;
		}
	}
}

} // namespace video
} // namespace fort
