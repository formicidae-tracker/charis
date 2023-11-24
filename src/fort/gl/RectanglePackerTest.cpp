
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "RectanglePackerTest.hpp"
#include "RectanglePacker.hpp"

namespace fort {
namespace gl {

void RectanglePackerTest::SetUp() {
	packer = std::make_unique<RectanglePacker<int>>(128, 128);
}

TEST_F(RectanglePackerTest, TooLargeIsUnfit) {
	auto res = packer->Place({129, 129});
	EXPECT_FALSE(res);
}

template <typename T>
auto PlacementEqual(const typename RectanglePacker<T>::Placement &arg) {
	using Placement = typename RectanglePacker<T>::Placement;
	return ::testing::AllOf(
	    ::testing::Field(
	        "Position",
	        &Placement::Position,
	        ::testing::Eq(arg.Position)
	    ),
	    ::testing::Field(
	        "Rotated",
	        &Placement::Rotated,
	        ::testing::Eq(arg.Rotated)
	    )
	);
}

TEST_F(RectanglePackerTest, PerfectOneFit) {
	auto res = packer->Place({128, 128})
	               .value_or(RectanglePacker<int>::Placement{
	                   .Position = {129, 129},
	                   .Rotated  = true,
	               });
	RectanglePacker<int>::Placement expected{
	    .Position = {0, 0},
	    .Rotated  = false,
	};
	EXPECT_THAT(res, PlacementEqual<int>(expected));
}

TEST_F(RectanglePackerTest, PerfectFit16) {

	for (size_t i = 0; i < 16; i++) {
		SCOPED_TRACE("iteration: " + std::to_string(i));

		auto res = packer->Place({32, 32});
		int  ix  = i / 4;
		int  iy  = i - ix * 4;

		RectanglePacker<int>::Placement expected{
		    .Position = {32 * ix, 32 * iy},
		    .Rotated  = false,
		};
		EXPECT_NO_THROW({
			EXPECT_THAT(res.value(), PlacementEqual<int>(expected));
		});
	}
}

TEST_F(RectanglePackerTest, ComplexFit) {

	auto res = packer->Place({32, 64});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {0, 0}, .Rotated = false})
		);
	});
	res = packer->Place({16, 32});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {0, 64}, .Rotated = false})
		);
	});

	res = packer->Place({32, 32});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {0, 96}, .Rotated = false})

		);
	});

	res = packer->Place({16, 80});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {32, 0}, .Rotated = false})

		);
	});

	res = packer->Place({16, 32});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {32, 96}, .Rotated = false})

		);
	});
	res = packer->Place({16, 32});
	EXPECT_NO_THROW({
		EXPECT_THAT(
		    res.value(),
		    PlacementEqual<int>({.Position = {16, 80}, .Rotated = true})

		);
	});
}

} // namespace gl
} // namespace fort
