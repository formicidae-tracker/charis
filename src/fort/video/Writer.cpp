#include "Writer.hpp"
#include "Encoder.hpp"
#include "details/AVCall.hpp"
#include <fort/utils/Defer.hpp>
#include <iostream>
#include <libavcodec/packet.h>
#include <libavutil/mathematics.h>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

std::ostream &operator<<(std::ostream &out, const AVRational &r) {
	return out << r.num << "/" << r.den;
}

namespace fort {
namespace video {

struct Writer::Implementation {

	using AVFormatContextPtr = std::
	    unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>>;

	AVFormatContextPtr       d_context;
	std::unique_ptr<Encoder> d_encoder;
	AVStream	            *d_stream;
	int64_t                  d_next = 0;

	Implementation(Params &&muxerParams, Encoder::Params &&encoderParams)
	    : d_context{nullptr, [](AVFormatContext *) {}}
	    , d_encoder{std::make_unique<Encoder>(std::move(encoderParams))} {
		open(muxerParams);
	}

	~Implementation() {
		using namespace fort::video::details;
		d_encoder->Flush();
		while (true) {
			auto pkt = d_encoder->Receive();

			if (!pkt) {
				break;
			}

			Write(pkt.get());
		}

		if ((d_context->oformat->flags & AVFMT_NOFILE) == 0 &&
		    d_context->pb != nullptr) {
			AVCall(av_write_trailer, d_context.get());
			avio_closep(&d_context->pb);
		}
	}

	void Write(AVPacket *pkt) {
		pkt->stream_index = d_stream->index;
		av_packet_rescale_ts(
		    pkt,
		    d_encoder->CodecContext()->time_base,
		    d_stream->time_base
		);
		details::AVCall(av_interleaved_write_frame, d_context.get(), pkt);
	}

	void Write(const Frame &frame) {
		d_encoder->Send(frame, d_next++);
		while (true) {
			auto pkt = d_encoder->Receive();

			if (!pkt) {
				break;
			}

			Write(pkt.get());
		}
	}

	void open(const Params &params) {
		using namespace fort::video::details;
		AVFormatContext *ctx{nullptr};

		AVCall(
		    avformat_alloc_output_context2,
		    &ctx,
		    nullptr,
		    nullptr,
		    params.Path.c_str()
		);

		d_context = AVFormatContextPtr{ctx, avformat_free_context};

		// we don't use a smart pointer as the stream is owned by the context.
		d_stream =
		    AVAlloc<AVStream>(avformat_new_stream, d_context.get(), nullptr);

		d_stream->time_base = d_encoder->CodecContext()->time_base;
		AVCall(
		    avcodec_parameters_from_context,
		    d_stream->codecpar,
		    d_encoder->CodecContext()
		);

		if (d_context->oformat->flags & AVFMT_GLOBALHEADER) {
			d_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		d_stream->id = d_context->nb_streams - 1;

		if ((d_context->oformat->flags & AVFMT_NOFILE) == 0) {
			AVCall(
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
			AVCall(
			    av_dict_set,
			    &opts,
			    params.MuxerOptionKey.c_str(),
			    params.MuxerOptionValue.c_str(),
			    0
			);
		}

		AVCall(avformat_write_header, d_context.get(), &opts);
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
