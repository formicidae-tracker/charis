#include "Writer.hpp"
#include "Encoder.hpp"
#include "details/AVCall.hpp"
#include <fort/utils/Defer.hpp>
#include <libavutil/dict.h>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

namespace fort {
namespace video {

struct Writer::Implementation {

	using AVFormatContextPtr = std::
	    unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>;

	AVFormatContextPtr       d_context;
	std::unique_ptr<Encoder> d_encoder;
	AVStream	            *d_stream;

	Implementation(Params &&muxerParams, Encoder::Params &&encoderParams)
	    : d_context{nullptr, [](AVFormatContext *) {}}
	    , d_encoder{std::make_unique<Encoder>(std::move(encoderParams))} {
		open(muxerParams);
	}

	void Write(AVPacket *pkt) {
		pkt->stream_index = d_stream->index;
		details::AVCall(av_interleaved_write_frame, d_context.get(), pkt);
	}

	void Write(const Frame &frame) {
		d_encoder->Send(frame);
		while (true) {
			auto pkt = d_encoder->Receive();
			if (!pkt) {
				break;
			}
			Write(pkt.get());
		}
	}

	void open(const Params &params) {

		AVFormatContext *ctx{nullptr};

		details::AVCall(
		    avformat_alloc_output_context2,
		    &ctx,
		    nullptr,
		    nullptr,
		    params.Path.c_str()
		);

		d_context = AVFormatContextPtr{
		    ctx,
		    [](AVFormatContext *ctx) {
			    if ((ctx->oformat->flags & AVFMT_NOFILE) == 0 &&
			        ctx->pb != nullptr) {
				    details::AVCall(av_write_trailer, ctx);
				    avio_closep(&ctx->pb);
			    }
			    avformat_free_context(ctx);
		    },
		};

		// we don't use a smart pointer as the stream is owned by the context.
		d_stream = details::AVAlloc<AVStream>(
		    avformat_new_stream,
		    d_context.get(),
		    nullptr
		);

		auto timebase       = d_encoder->Timebase();
		d_stream->time_base = AVRational{timebase.Num, timebase.Den};

		if (d_context->oformat->flags & AVFMT_GLOBALHEADER) {
			d_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		d_stream->id = d_context->nb_streams - 1;

		if ((d_context->oformat->flags & AVFMT_NOFILE) == 0) {
			details::AVCall(
			    avio_open,
			    &d_context->pb,
			    params.Path.c_str(),
			    AVIO_FLAG_WRITE
			);
		}

		AVDictionary *opts{nullptr};
		defer {
			if (opts != nullptr) {
				av_dict_free(&opts);
			}
		};
		if (!params.MuxerOptionKey.empty() &&
		    !params.MuxerOptionValue.empty()) {
			details::AVCall(
			    av_dict_set,
			    &opts,
			    params.MuxerOptionKey.c_str(),
			    params.MuxerOptionValue.c_str(),
			    0
			);
		}

		details::AVCall(avformat_write_header, d_context.get(), &opts);
	}
};

Writer::Writer(Params &&muxerParams, Encoder::Params &&encoderParams)
    : self{std::make_unique<Implementation>(
          std::move(muxerParams), std::move(encoderParams)
      )} {}

Writer::~Writer() = default;

void Writer::Write(AVPacket *pkt) {
	self->Write(pkt);
}

void Writer::Write(const Frame &frame) {
	self->Write(frame);
}

} // namespace video
} // namespace fort
