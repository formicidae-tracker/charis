cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0048 NEW)

project(
	charis
	LANGUAGES C CXX
	VERSION 0.0.1
)

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CHARIS_IMPORTED 1)
endif(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FORT_CHARIS_BUILD_TIME "Builds libfort-time library" On)
option(FORT_CHARIS_BUILD_GL "Builds libfort-gl library" On)
option(FORT_CHARIS_BUILD_UTILS "Builds libfort-utils library" On)
option(FORT_CHARIS_BUILD_VIDEO "Builds libfort-video library" On)
option(FORT_CHARIS_BUILD_OPTIONS "Builds libfort-options library" On)
option(FORT_CHARIS_USE_MAGIC_ENUM "Uses magic_enum" On)
option(FORT_CHARIS_USE_FKYAML "Uses FK Yaml" On)

include(FetchContent)

if(NOT CHARIS_IMPORTED)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG 9bb354fa8325fa31faa1e12627b25ab445e6eed3
	)
	# For Windows: Prevent overriding the parent project's compiler/linker
	# settings
	set(gtest_force_shared_crt
		ON
		CACHE BOOL "" FORCE
	)

	FetchContent_MakeAvailable(googletest)
	enable_testing()

	include(GoogleTest)

	add_custom_target(check ALL ${CMAKE_CTEST_COMMAND} ARGS --output-on-failure)

endif(NOT CHARIS_IMPORTED)

add_subdirectory(src/fort)
