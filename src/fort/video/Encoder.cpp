#include <cpptrace/cpptrace.hpp>

#include <fort/utils/ObjectPool.hpp>
#include <fort/video/Types.hpp>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include "Encoder.hpp"
#include "TypesIO.hpp"
#include "details/AVCall.hpp"
#include "details/AVTypes.hpp"
#include <iostream>

namespace fort {
namespace video {

struct Encoder::Implementation {

	PixelFormat d_expectedFormat;
	using PacketPool = utils::ObjectPool<
	    AVPacket,
	    std::function<AVPacket *()>,
	    std::function<void(AVPacket *)>>;

	details::AVFramePtr d_frame = details::AVFramePtr{av_frame_alloc()};
	PacketPool::Ptr     d_pool =
	    PacketPool::Create(av_packet_alloc, [](AVPacket *pkt) {
		    av_packet_free(&pkt);
	    });
	details::AVCodecContextPtr d_codec;
	details::SwsContextPtr     d_scale;

	Implementation(Encoder::Params &&params)
	    : d_expectedFormat{params.Format} {
		using namespace fort::video::details;

		auto enc = avcodec_find_encoder_by_name(params.Codec.c_str());
		if (enc == nullptr) {
			throw cpptrace::runtime_error{
			    "could not found codec '" + params.Codec + "'",
			};
		}

		d_codec            = MakeAVCodecContext(enc);
		d_codec->width     = std::get<0>(params.Size);
		d_codec->height    = std::get<1>(params.Size);
		d_codec->time_base = {params.Framerate.Den, params.Framerate.Num};
		d_codec->pix_fmt   = AV_PIX_FMT_YUV420P;
		d_codec->bit_rate  = params.BitRate;
		d_codec->rc_buffer_size =
		    std::max(2 * params.BitRate, params.MaxBitRate);
		d_codec->rc_min_rate = params.MinBitRate;
		d_codec->rc_max_rate = params.MaxBitRate;

		AVCall(avcodec_open2, d_codec.get(), enc, nullptr);

		AVCall(
		    av_opt_set,
		    d_codec->priv_data,
		    params.ParamID.c_str(),
		    params.ParamKeyValues.c_str(),
		    0
		);

		d_frame->format = d_codec->pix_fmt;
		d_frame->width  = d_codec->width;
		d_frame->height = d_codec->height;

		AVCall(av_frame_get_buffer, d_frame.get(), 0);

		if (params.Format != AV_PIX_FMT_YUV420P) {
			d_scale = SwsContextPtr{sws_getContext(
			    d_codec->width,
			    d_codec->height,
			    params.Format,
			    d_codec->width,
			    d_codec->height,
			    AV_PIX_FMT_YUV420P,
			    SWS_BILINEAR,
			    nullptr,
			    nullptr,
			    nullptr
			)};
		}
	}

	PacketPool::ObjectPtr Receive() {
		auto pkt   = d_pool->Get(av_packet_unref);
		int  error = avcodec_receive_packet(d_codec.get(), pkt.get());
		if (error == AVERROR(EAGAIN) || error == AVERROR_EOF) {
			return nullptr;
		} else if (error < 0) {
			throw details::AVError(error, avcodec_receive_frame);
		}

		return pkt;
	}

	void Send(const Frame &f, int64_t pts) {
		if (f.Format != d_expectedFormat ||
		    f.Size != Resolution{d_frame->width, d_frame->height}) {
			char current[200], expected[200];
			av_get_pix_fmt_string(current, 200, f.Format);
			av_get_pix_fmt_string(expected, 200, d_expectedFormat);

			throw std::invalid_argument{
			    std::string{"invalid input frame {format: "} + current +
			    ", size: " + std::to_string(f.Size) +
			    "}, expected {format: " + expected + ", size: " +
			    std::to_string(Resolution{d_frame->width, d_frame->height})};
		}

		details::AVCall(av_frame_make_writable, d_frame.get());
		if (d_scale) {
			details::AVCall(
			    sws_scale,
			    d_scale.get(),
			    f.Planes,
			    f.Linesize,
			    0,
			    d_codec->height,
			    d_frame->data,
			    d_frame->linesize
			);
		} else {
			av_image_copy(
			    d_frame->data,
			    d_frame->linesize,
			    const_cast<const uint8_t **>(f.Planes),
			    f.Linesize,
			    AV_PIX_FMT_YUV420P,
			    d_codec->width,
			    d_codec->height
			);
		}
		d_frame->pts = pts;
		details::AVCall(avcodec_send_frame, d_codec.get(), d_frame.get());
	}

	void Flush() {
		details::AVCall(avcodec_send_frame, d_codec.get(), nullptr);
	}
};

Encoder::Encoder(Params &&params)
    : self{std::make_unique<Encoder::Implementation>(std::move(params))} {}

Encoder::~Encoder() = default;

void Encoder::Send(const Frame &frame, int64_t pts) {
	self->Send(frame, pts);
}

AVCodecContext *Encoder::CodecContext() const {
	return self->d_codec.get();
}

std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> Encoder::Receive() {
	return self->Receive();
}

void Encoder::Flush() {
	self->Flush();
}

} // namespace video
} // namespace fort
