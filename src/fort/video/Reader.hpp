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
	using FramePtr = std::unique_ptr<Frame, std::function<void(Frame *)>>;

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

	FramePtr Grab();

private:
	struct Implementation;

	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
