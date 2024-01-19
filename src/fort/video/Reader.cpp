#include "Reader.hpp"
#include <google/protobuf/descriptor.h>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <functional>
#include <queue>
#include <utility>

#include <cpptrace/cpptrace.hpp>

#include <fort/utils/Defer.hpp>
#include <fort/utils/ObjectPool.hpp>

#include "Frame.hpp"
#include "Types.hpp"
#include "TypesIO.hpp"
#include "details/AVCall.hpp"
#include "details/AVTypes.hpp"

#include <iostream>

namespace fort {
namespace video {

namespace details {
template <typename Frame> struct FrameOrderer {
	constexpr bool operator()(const Frame &a, const Frame &b) noexcept {
		return a->Index > b->Index;
	}
};
} // namespace details

struct Reader::Implementation {
	using AVFormatContextPtr =
	    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)>;

	AVFormatContextPtr d_context = {nullptr, nullptr};
	int                d_index   = -1;

	details::AVCodecContextPtr d_codec;

	details::AVPacketPtr d_packet = details::AVPacketPtr{av_packet_alloc()};
	details::AVFramePtr  d_frame  = details::AVFramePtr{av_frame_alloc()};

	details::SwsContextPtr d_scaleContext;

	PixelFormat          d_format = AV_PIX_FMT_GRAY8;
	Resolution           d_size;

	size_t d_next   = 0;
	bool   d_queued = false;

	Implementation(
	    const std::filesystem::path &path,
	    PixelFormat                  format,
	    std::tuple<int, int>         targetSize
	)
	    : d_context{open(path), [](AVFormatContext *c) {
		                if (c) {
			                avformat_close_input(&c);
		                }
	    }}
	    ,d_format{format} {
		using namespace fort::video::details;
		// Needed for mpeg2 formats
		AVCall(avformat_find_stream_info, d_context.get(), nullptr);

		d_index = AVCall(
		    av_find_best_stream,
		    d_context.get(),
		    AVMEDIA_TYPE_VIDEO,
		    -1,
		    -1,
		    nullptr,
		    0
		);

		auto dec = avcodec_find_decoder(Stream()->codecpar->codec_id);
		if (dec == nullptr) {
			throw cpptrace::runtime_error{
			    "could not find video codec " +
			        std::to_string(Stream()->codecpar->codec_id),
			};
		}

		d_codec = details::MakeAVCodecContext(dec);

		AVCall(
		    avcodec_parameters_to_context,
		    d_codec.get(),
		    Stream()->codecpar
		);
		AVCall(avcodec_open2, d_codec.get(), dec, nullptr);

		auto [outputWidth, outputHeight] = targetSize;
		if (outputWidth <= 0 || outputHeight <= 0) {
			outputWidth  = d_codec->width;
			outputHeight = d_codec->height;
		}
		d_size = {outputWidth, outputHeight};
		if (d_codec->width != outputWidth || d_codec->height != outputHeight ||
		    d_codec->pix_fmt != format) {
			d_scaleContext = SwsContextPtr{sws_getContext(
			    d_codec->width,
			    d_codec->height,
			    d_codec->pix_fmt,
			    outputWidth,
			    outputHeight,
			    d_format,
			    SWS_BILINEAR,
			    nullptr,
			    nullptr,
			    nullptr
			)};
		}
	}

	bool Grab(bool checkIFrame = false) {
		using namespace fort::video::details;
		if (d_packet) {
			int error = av_read_frame(d_context.get(), d_packet.get());
			if (error < 0) {
				if (error != AVERROR_EOF) {
					throw AVError(error, av_read_frame);
				} else {
					d_packet.reset();
					AVCall(avcodec_send_packet, d_codec.get(), nullptr);
				}
			}
		}

		defer {
			if (d_packet) {
				av_packet_unref(d_packet.get());
			}
		};

		if (d_queued == true) {
			av_frame_unref(d_frame.get());
			d_queued = false;
		}

		if (d_packet) {
			AVCall(avcodec_send_packet, d_codec.get(), d_packet.get());
		}

		int error = avcodec_receive_frame(d_codec.get(), d_frame.get());
		if (error == AVERROR(EAGAIN)) {
			return Grab(checkIFrame);
		} else if (error == AVERROR_EOF) {
			return false;
		} else if (error < 0) {
			throw AVError(error, avcodec_receive_frame);
		}

		if (checkIFrame && d_frame->pict_type != AV_PICTURE_TYPE_I) {
			throw cpptrace::runtime_error(
			    std::string("Only I-Frame requested, but received a ") +
			    av_get_picture_type_char(d_frame->pict_type)
			);
		}
		d_queued = true;
		return true;
	}

	bool Receive(Frame &frame) {
		if (d_queued == false) {
			return false;
		}
		defer {
			d_queued = false;
			av_frame_unref(d_frame.get());
		};

		if (frame.Format != d_format || frame.Size != d_size) {
			throw cpptrace::invalid_argument{
			    "invalid frame format " + std::to_string(frame.Format) + " " +
			    std::to_string(frame.Size)};
		}

		if (d_scaleContext) {
			details::AVCall(
			    sws_scale,
			    d_scaleContext.get(),
			    d_frame->data,
			    d_frame->linesize,
			    0,
			    d_codec->height,
			    frame.Planes,
			    frame.Linesize
			);

		} else {
			av_image_copy(
			    frame.Planes,
			    frame.Linesize,
			    const_cast<const uint8_t **>(d_frame->data),
			    d_frame->linesize,
			    d_format,
			    d_codec->width,
			    d_codec->height
			);
		}
		frame.PTS   = FramePTS(*d_frame);
		frame.Index = FrameIndex(*d_frame);
		d_next      = frame.Index + 1;
		return true;
	}

	AVStream *Stream() const noexcept {
		return d_context->streams[d_index];
	}

	AVFormatContext *open(const std::filesystem::path &path) {
		AVFormatContext *ctx = nullptr;
		details::AVCall(
		    avformat_open_input,
		    &ctx,
		    path.string().c_str(),
		    nullptr,
		    nullptr
		);
		return ctx;
	}

	void seek(
	    int64_t                                                        position,
	    int                                                            flags,
	    std::function<bool(const Implementation &, const AVFrame &)> &&predicate
	) {
		details::AVCall(
		    avformat_seek_file,
		    d_context.get(),
		    d_index,
		    0, // std::numeric_limits<int64_t>::min(),
		    position,
		    position,
		    flags | AVSEEK_FLAG_BACKWARD
		);

		avcodec_flush_buffers(d_codec.get());

		if (!d_packet) {
			d_packet = details::AVPacketPtr{av_packet_alloc()};
		}

		bool checkIFrame = true;
		bool res         = false;
		do {
			res         = Grab(checkIFrame);
			checkIFrame = false;
		} while (res && predicate(*this, *d_frame));
		d_queued = res;
	}

	size_t Position() const noexcept {
		if (d_queued == true) {
			return av_rescale_q(
			    d_frame->pts,
			    Stream()->time_base,
			    {Stream()->avg_frame_rate.den, Stream()->avg_frame_rate.num}
			);
		}
		return d_next;
	}

	size_t FrameIndex(const AVFrame &frame) const noexcept {
		return av_rescale_q(
		    frame.pts,
		    Stream()->time_base,
		    {Stream()->avg_frame_rate.den, Stream()->avg_frame_rate.num}
		);
	}

	video::Duration FramePTS(const AVFrame &frame) const noexcept {
		return video::Duration(
		    av_rescale_q(d_frame->pts, Stream()->time_base, {1, int64_t(1e9)})
		);
	}
};

Reader::Reader(
    const std::filesystem::path &path,
    PixelFormat                  format,
    std::tuple<int, int>         targetSize
)
    : self{std::make_unique<Implementation>(path, format, targetSize)} {}

Reader::~Reader() = default;

Resolution Reader::Size() const noexcept {
	const auto codecpar = self->Stream()->codecpar;
	return Resolution{codecpar->width, codecpar->height};
}

Duration Reader::Duration() const noexcept {
	return video::Duration{
	    av_rescale_q(
	        self->d_context->duration,
	        {1, AV_TIME_BASE},
	        {1, int64_t(1e9)}
	    ),
	};
}

Duration Reader::AverageFrameDuration() const noexcept {
	std::cerr << self->Stream()->time_base.num << "/"
	          << self->Stream()->time_base.den << std::endl;
	return video::Duration{av_rescale(
	    self->Stream()->avg_frame_rate.den,
	    1e9,
	    self->Stream()->avg_frame_rate.num
	)};
}

size_t Reader::Length() const noexcept {
	return self->Stream()->nb_frames;
}

bool Reader::Read(Frame &frame) {
	// Frame already grabbed, just receive it.
	if (self->d_queued) {
		return Receive(frame);
	}

	// try to grab it.
	if (!Grab()) {
		return false;
	}
	// receive it.
	return Receive(frame);
}

bool Reader::Grab() {
	return self->Grab();
}

bool Reader::Receive(Frame &frame) {
	return self->Receive(frame);
}

void Reader::SeekFrame(size_t position) {
	self->seek(
	    position,
	    AVSEEK_FLAG_FRAME,
	    [position](const Implementation &self, const AVFrame &f) {
		    return self.FrameIndex(f) < position;
	    }
	);
}

void Reader::SeekTime(video::Duration duration) {
	self->seek(
	    duration.count(),
	    0,
	    [pts = duration.count()](const Implementation &self, const AVFrame &f) {
		    return self.FramePTS(f).count() < pts;
	    }
	);
}

size_t Reader::Position() const noexcept {
	return self->Position();
}

std::unique_ptr<video::Frame> Reader::CreateFrame(int alignement) const {
	return std::make_unique<video::Frame>(
	    self->d_size,
	    self->d_format,
	    alignement
	);
}
} // namespace video
} // namespace fort
