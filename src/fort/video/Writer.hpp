#pragma once

#include <filesystem>

#include "Encoder.hpp"
#include "Frame.hpp"

namespace fort {
namespace video {

class Writer {
public:
	struct Params {
		std::filesystem::path Path;
		std::string           MuxerOptionKey;
		std::string           MuxerOptionValue;
	};

	Writer(Params &&muxerParams, Encoder::Params &&encoderParams);
	~Writer();

	void Write(AVPacket *pkt);
	void Write(const Frame &frame);

private:
	struct Implementation;
	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
