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
		std::tuple<int, int> Size;
		std::string          Codec   = "libx264";
		std::string          ParamID = "x264-params";
		std::string          ParamKeyValues =
		    "keyint=60:min-keyint=60:scenecut=0:force-cfr=1";
		Ratio<int>  Framerate;
		PixelFormat Format = AV_PIX_FMT_YUV420P;
	};

	Encoder(Params &&params);

	~Encoder();

	void Send(const Frame &frame);
	void Flush();
	Ratio<int> Timebase() const noexcept;

	std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> Receive();

private:
	friend class Writer;
	struct Implementation;
	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
