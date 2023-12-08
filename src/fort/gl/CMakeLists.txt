include(add_resources)

include_directories(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
                    ${CMAKE_CURRENT_BINARY_DIR})

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

add_resources(
  TARGET fort-gl #
  RESOURCES shaders/text.fragment #
  shaders/text.vertex #
)

set_target_properties(
  fort-gl
  PROPERTIES VERSION ${PROJECT_VERSION}
             SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

target_link_libraries(
  fort-gl
  GLEW::glew
  glfw
  OpenGL::GL
  Eigen3::Eigen
  slog++::slog++
  concurrentqueue
  Fontconfig::Fontconfig
  Freetype::Freetype)

if(NOT CHARIS_IMPORTED)
  set(TEST_SRC_FILES #
      RectanglePackerTest.cpp #
  )

  set(TEST_HDR_FILES #
      RectanglePackerTest.hpp #
  )

  add_executable(charis-gl-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
  target_link_libraries(charis-gl-tests fort-gl GTest::gtest_main
                        GTest::gmock_main Eigen3::Eigen)
  gtest_discover_tests(charis-gl-tests)
  add_dependencies(check charis-gl-tests)
else(NOT CHARIS_IMPORTED)
  add_library(fort-charis::gl INTERFACE IMPORTED GLOBAL)
  target_link_libraries(
    fort-charis::libfort-gl INTERFACE fort-gl GLEW::glew glfw OpenGL::GL
                                      Eigen3::Eigen)
  target_include_directories(fort-charis::libfort-gl
                             INTERFACE ${PROJECT_SOURCE_DIR}/src)

endif(NOT CHARIS_IMPORTED)
