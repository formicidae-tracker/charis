include(add_resources)

include_directories(
	${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}
)

set(SRC_FILES #
	Window.cpp #
	Shader.cpp #
	RectanglePacker.cpp #
	FontAtlas.cpp #
	VAOPool.cpp #
	TextRenderer.cpp #
)

set(HDR_FILES #
	Window.hpp #
	Shader.hpp #
	RectanglePacker.hpp #
	FontAtlas.hpp #
	VAOPool.hpp #
	TextRenderer.hpp #
)

add_library(fort-gl SHARED ${SRC_FILES} ${HDR_FILES})
add_library(fort-charis::libfort-gl ALIAS fort-gl)

set_target_properties(
	fort-gl
	PROPERTIES VERSION ${PROJECT_VERSION}
			   SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

add_resources(
	TARGET
	fort-gl #
	RESOURCES
	shaders/text.fragment #
	shaders/text.vertex #
	shaders/background.fragment #
)

target_include_directories(fort-gl INTERFACE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(
	fort-gl
	PUBLIC GLEW::glew glfw OpenGL::GL Eigen3::Eigen slog++::slog++
		   concurrentqueue
	PRIVATE Fontconfig::Fontconfig Freetype::Freetype
)

if(NOT CHARIS_IMPORTED)
	set(TEST_SRC_FILES #
		RectanglePackerTest.cpp #
	)

	set(TEST_HDR_FILES #
		RectanglePackerTest.hpp #
	)

	add_executable(charis-gl-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
	target_link_libraries(
		charis-gl-tests fort-charis::libfort-gl GTest::gtest_main
		GTest::gmock_main
	)
	gtest_discover_tests(charis-gl-tests)
	add_dependencies(check charis-gl-tests)
endif(NOT CHARIS_IMPORTED)

install(FILES ${HDR_FILES} DESTINATION include/fort/gl)
install(TARGETS fort-gl DESTINATION lib)
