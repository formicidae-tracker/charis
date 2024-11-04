include_directories(${PROJECT_SOURCE_DIR}/src)

set(HDR_FILES
	Reader.hpp
	Frame.hpp
	details/AVCall.hpp
	details/AVTypes.hpp
	Writer.hpp
	Encoder.hpp
	PNG.hpp
)
set(SRC_FILES Reader.cpp Frame.cpp Writer.cpp Encoder.cpp PNG.cpp)

add_library(fort-video SHARED ${SRC_FILES} ${HDR_FILES})

target_link_libraries(
	fort-video PkgConfig::ffmpeg cpptrace::cpptrace fort-charis::libfort-utils
	spng "-rdynamic"
)

if(NOT CHARIS_IMPORTED)
	set(TEST_SRC_FILES ReaderTest.cpp WriterTest.cpp details/SPNGCallTest.cpp
					   details/AVCallTest.cpp PNGTest.cpp
	)
	set(TEST_HDR_FILES)
	add_executable(charis-video-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
	target_link_libraries(charis-video-tests fort-video GTest::gtest_main)

	add_test(NAME fort-video COMMAND charis-video-tests)
	add_dependencies(check charis-video-tests)
else(NOT CHARIS_IMPORTED)
	add_library(fort-charis::libfort-video INTERFACE IMPORTED GLOBAL)
	target_link_libraries(fort-charis::libfort-video INTERFACE fort-video)
	target_include_directories(
		fort-charis::libfort-video INTERFACE ${PROJECT_SOURCE_DIR}/src
	)
endif(NOT CHARIS_IMPORTED)

install(FILES ${HDR_FILES} DESTINATION include/fort/video)
install(TARGETS fort-video DESTINATION lib)
