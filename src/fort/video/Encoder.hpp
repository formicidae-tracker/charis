#pragma once

#include <memory>
#include <string>

#include "Frame.hpp"
#include "Types.hpp"

namespace fort {
namespace video {

class Encoder {
public:
	struct Params {
		Resolution  Size;
		std::string Codec          = "libx264";
		std::string ParamID        = "x264-params";
		std::string ParamKeyValues = "keyint=60:min-keyint=60:scenecut=0";
		Ratio<int>  Framerate;
		PixelFormat Format = AV_PIX_FMT_YUV420P;

		int64_t BitRate    = 2 * 1024 * 1024;
		int64_t MinBitRate = 500 * 1024;
		int64_t MaxBitRate = 3 * 1024 * 1024;
	};

	Encoder(Params &&params);

	~Encoder();

	void Send(const Frame &frame, int64_t pts);
	void Flush();

	std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> Receive();

private:
	friend class Writer;

	AVCodecContext *CodecContext() const;

	struct Implementation;
	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
