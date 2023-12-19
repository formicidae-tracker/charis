include_directories(${PROJECT_SOURCE_DIR}/src)

set(SRC_FILES Reader.hpp Frame.hpp details/AVCall.hpp)
set(HDR_FILES Reader.cpp Frame.cpp)

add_library(fort-video SHARED ${SRC_FILES} ${HDR_FILES})

target_link_libraries(fort-video PkgConfig::ffmpeg cpptrace::cpptrace)

if(NOT CHARIS_IMPORTED)
	set(TEST_SRC_FILES ReaderTest.cpp)
	set(TEST_HDR_FILES)
	add_executable(charis-video-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
	target_link_libraries(charis-video-tests fort-video GTest::gtest_main)

	gtest_discover_tests(charis-video-tests)
	add_dependencies(check charis-video-tests)
else(NOT CHARIS_IMPORTED)
	add_library(fort-charis::libfort-video INTERFACE IMPORTED GLOBAL)
	target_link_libraries(fort-charis::libfort-video INTERFACE fort-video)
	target_include_directories(
		fort-charis::libfort-video INTERFACE ${PROJECT_SOURCE_DIR}/src
	)
endif(NOT CHARIS_IMPORTED)
