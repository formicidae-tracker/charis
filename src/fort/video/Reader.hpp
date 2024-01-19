#pragma once

#include "Frame.hpp"
#include "Types.hpp"
#include <filesystem>
#include <functional>
#include <tuple>

namespace fort {
namespace video {

class Reader {
public:
	Reader(
	    const std::filesystem::path &path,
	    PixelFormat                     = AV_PIX_FMT_GRAY8,
	    std::tuple<int, int> targetSize = {-1, -1}
	);

	~Reader();

	std::tuple<int, int> Size() const noexcept;

	video::Duration Duration() const noexcept;

	size_t Length() const noexcept;

	size_t Position() const noexcept;

	video::Duration AverageFrameDuration() const noexcept;

	void SeekFrame(size_t position);

	void SeekTime(video::Duration duration);

	bool Grab();

	bool Receive(Frame &frame);

	bool Read(Frame &frame);

	std::unique_ptr<video::Frame> CreateFrame() const;

private:
	struct Implementation;

	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
