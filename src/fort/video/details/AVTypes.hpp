#pragma once
#include <cpptrace/cpptrace.hpp>

#include <fort/video/Types.hpp>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
#include <memory>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
}

namespace fort {
namespace video {
namespace details {

struct FreeCodecContext {
	void operator()(AVCodecContext *ctx) const {
		avcodec_free_context(&ctx);
	}
};

using AVCodecContextPtr = std::unique_ptr<AVCodecContext, FreeCodecContext>;

inline AVCodecContextPtr MakeAVCodecContext(const AVCodec *codec) {
	auto res = avcodec_alloc_context3(codec);
	if (res == nullptr) {
		throw cpptrace::runtime_error{
		    "could not allocate coded " + std::string(codec->long_name)};
	}
	return AVCodecContextPtr{res};
}

struct FreeAVPacket {
	void operator()(AVPacket *pkt) const {
		av_packet_free(&pkt);
	}
};

using AVPacketPtr = std::unique_ptr<AVPacket, FreeAVPacket>;

struct FreeAVFrame {
	void operator()(AVFrame *frame) const {
		av_frame_free(&frame);
	}
};

using AVFramePtr = std::unique_ptr<AVFrame, FreeAVFrame>;

struct FreeSwsContext {
	void operator()(SwsContext *ctx) const {
		sws_freeContext(ctx);
	}
};

using SwsContextPtr = std::unique_ptr<SwsContext, FreeSwsContext>;

} // namespace details
} // namespace video
} // namespace fort
