#pragma once

#include <filesystem>
#include <fort/video/Frame.hpp>
#include <fort/video/Types.hpp>
#include <memory>

namespace fort {

namespace video {

std::unique_ptr<video::Frame>
ReadPNG(const std::filesystem::path &path, PixelFormat = AV_PIX_FMT_GRAY8);

void WritePNG(const std::filesystem::path &path, const video::Frame &image);

} // namespace video
} // namespace fort
