#pragma once

#include <gtest/gtest.h>

#include "RectanglePacker.hpp"

namespace fort {
namespace gl {
class RectanglePackerTest : public testing::Test {
protected:
	void SetUp();

	std::unique_ptr<RectanglePacker<int>> packer;
};

} // namespace gl
} // namespace fort
