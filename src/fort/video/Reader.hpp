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

	Resolution Size() const noexcept;

	video::Duration Duration() const noexcept;

	size_t Length() const noexcept;

	size_t Position() const noexcept;

	video::Duration AverageFrameDuration() const noexcept;

	size_t SeekFrame(size_t position, bool advance = true);

	video::Duration SeekTime(video::Duration duration, bool advance = true);

	bool Grab();

	bool Receive(Frame &frame);

	bool Read(Frame &frame);

	std::unique_ptr<video::Frame> CreateFrame(int alignement = 32) const;

private:
	struct Implementation;

	std::unique_ptr<Implementation> self;
};

} // namespace video
} // namespace fort
