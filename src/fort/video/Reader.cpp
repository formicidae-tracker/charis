#include "Reader.hpp"

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
#include "details/AVCall.hpp"

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
	using FramePool = utils::ObjectPool<Frame, std::function<Frame *()>>;

	using AVFormatContextPtr =
	    std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)>;

	using AVCodecContextPtr =
	    std::unique_ptr<AVCodecContext, void (*)(AVCodecContext *)>;

	using AVPacketPtr = std::unique_ptr<AVPacket, void (*)(AVPacket *)>;

	using AVFramePtr = std::unique_ptr<AVFrame, void (*)(AVFrame *)>;

	using SwsContextPtr = std::unique_ptr<SwsContext, void (*)(SwsContext *)>;

	using FrameQueue = std::priority_queue<
	    FramePool::ObjectPtr,
	    std::vector<FramePool::ObjectPtr>,
	    details::FrameOrderer<FramePool::ObjectPtr>>;

	AVFormatContextPtr d_context = {nullptr, nullptr};
	int                d_index   = -1;
	AVCodecContextPtr  d_codec   = {nullptr, nullptr};
	AVPacketPtr        d_packet  = {
	            av_packet_alloc(),
	            [](AVPacket *pkt) { av_packet_free(&pkt); },
    };
	SwsContextPtr d_scaleContext = {nullptr, nullptr};
	AVFramePtr    d_frame        = {
	              av_frame_alloc(),
	              [](AVFrame *frame) { av_frame_free(&frame); },
    };
	FramePool::Ptr d_imagePool;
	FrameQueue     d_queue;
	PixelFormat    d_format = AV_PIX_FMT_GRAY8;

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

		auto decCtx = avcodec_alloc_context3(dec);
		if (decCtx == nullptr) {
			throw cpptrace::runtime_error{
			    "could not allocate  context for codec " +
			    std::string(dec->long_name)};
		}
		d_codec = {decCtx, [](AVCodecContext *ctx) {
			           if (ctx) {
				           avcodec_free_context(&ctx);
			           }
		           }};

		AVCall(avcodec_parameters_to_context, decCtx, Stream()->codecpar);
		AVCall(avcodec_open2, decCtx, dec, nullptr);

		auto [outputWidth, outputHeight] = targetSize;
		if (outputWidth <= 0 || outputHeight <= 0) {
			outputWidth  = d_codec->width;
			outputHeight = d_codec->height;
		}

		d_imagePool = FramePool::Create(
		    [w = outputWidth, h = outputHeight, pixelFormat = d_format]() {
			    auto res = new Frame{
			        .Format = pixelFormat,
			        .Size   = {w, h},
			    };
			    AVCall(
			        av_image_alloc,
			        res->Planes,
			        res->Linesize,
			        w,
			        h,
			        pixelFormat,
			        16
			    );
			    return res;
		    }
		);

		if (d_codec->width != outputWidth || d_codec->height != outputHeight ||
		    d_codec->pix_fmt != format) {
			d_scaleContext = {
			    sws_getContext(
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
			    ),
			    sws_freeContext,
			};
		}
	}

	FramePool::ObjectPtr Grab(bool checkIFrame = false) {
		using namespace fort::video::details;
		FramePool::ObjectPtr res = nullptr;
		if (!d_queue.empty()) {
			res = std::move(const_cast<FramePool::ObjectPtr &>(d_queue.top()));
			d_queue.pop();
			return res;
		}
		if (!d_packet) {
			return nullptr;
		}

		try {
			AVCall(av_read_frame, d_context.get(), d_packet.get());
		} catch (const AVError &e) {
			if (e.Code() != AVERROR_EOF) {
				throw;
			} else {
				d_packet.reset();
			}
		}

		defer {
			if (d_packet) {
				av_packet_unref(d_packet.get());
			}
		};

		AVCall(avcodec_send_packet, d_codec.get(), d_packet.get());

		while (true) {
			try {
				AVCall(avcodec_receive_frame, d_codec.get(), d_frame.get());
			} catch (const AVError &e) {
				if (e.Code() == AVERROR(EAGAIN) || e.Code() == AVERROR_EOF) {
					break;
				}
				throw;
			}

			defer {
				av_frame_unref(d_frame.get());
			};

			if (checkIFrame && d_frame->pict_type != AV_PICTURE_TYPE_I) {
				throw cpptrace::runtime_error(
				    std::string("Only I-Frame requested, but received a ") +
				    av_get_picture_type_char(d_frame->pict_type)
				);
			}

			auto newFrame = d_imagePool->Get();

			if (d_scaleContext) {
				AVCall(
				    sws_scale,
				    d_scaleContext.get(),
				    d_frame->data,
				    d_frame->linesize,
				    0,
				    d_codec->height,
				    newFrame->Planes,
				    newFrame->Linesize
				);
			} else {
				av_image_copy(
				    newFrame->Planes,
				    newFrame->Linesize,
				    const_cast<const uint8_t **>(d_frame->data),
				    d_frame->linesize,
				    d_format,
				    d_codec->width,
				    d_codec->height
				);
			}

			newFrame->PTS   = video::Duration(av_rescale_q(
                d_frame->pts,
                Stream()->time_base,
                {1, int64_t(1e9)}
            ));
			newFrame->Index = av_rescale_q(
			    d_frame->pts,
			    Stream()->time_base,
			    {Stream()->avg_frame_rate.den, Stream()->avg_frame_rate.num}
			);

			d_queue.push(std::move(newFrame));
		}

		if (!d_queue.empty()) {
			res = std::move(const_cast<FramePool::ObjectPtr &>(d_queue.top()));
			d_queue.pop();
			return res;
		}

		return Grab(checkIFrame);
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
	    int64_t                              position,
	    int                                  flags,
	    std::function<bool(const Frame &)> &&predicate
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
			d_packet = {av_packet_alloc(), [](AVPacket *pkt) {
				            av_packet_free(&pkt);
			            }};
		}

		FramePool::ObjectPtr frame;

		bool checkIFrame = true;
		do {
			frame       = Grab(checkIFrame);
			checkIFrame = false;
		} while (frame && predicate(*frame));
		while (!d_queue.empty()) {
			d_queue.pop();
		}
		if (frame) {
			d_queue.push(std::move(frame));
		}
	}
};

Reader::Reader(
    const std::filesystem::path &path,
    PixelFormat                  format,
    std::tuple<int, int>         targetSize
)
    : self{std::make_unique<Implementation>(path, format, targetSize)} {}

Reader::~Reader() = default;

std::tuple<int, int> Reader::Size() const noexcept {
	const auto codecpar = self->Stream()->codecpar;
	return {codecpar->width, codecpar->height};
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

size_t Reader::Length() const noexcept {
	return self->Stream()->nb_frames;
}

std::unique_ptr<Frame, std::function<void(Frame *)>> Reader::Grab() {
	return self->Grab();
}

void Reader::SeekFrame(size_t position) {
	self->seek(position, AVSEEK_FLAG_FRAME, [position](const Frame &f) {
		return f.Index < position;
	});
}

void Reader::SeekTime(video::Duration duration) {
	self->seek(duration.count(), 0, [pts = duration.count()](const Frame &f) {
		return f.PTS.count() < pts;
	});
}

} // namespace video
} // namespace fort
