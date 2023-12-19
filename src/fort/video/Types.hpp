#pragma once

#include <chrono>
#include <ratio>

namespace fort {
namespace video {

using Duration = std::chrono::duration<int64_t, std::nano>;
} // namespace video
} // namespace fort
