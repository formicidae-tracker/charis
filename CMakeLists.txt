cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0048 NEW)

project(
  charis
  LANGUAGES C CXX
  VERSION 0.0.1)

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(CHARIS_IMPORTED 1)
endif(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  slogpp
  GIT_REPOSITORY https://github.com/atuleu/slogpp.git
  GIT_TAG 525bd84d12e4b16916344da83fa73646d7deb957)

FetchContent_Declare(
  concurrentqueue
  GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
  GIT_TAG v1.0.4)

if(NOT CHARIS_IMPORTED)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG 9bb354fa8325fa31faa1e12627b25ab445e6eed3)
  # For Windows: Prevent overriding the parent project's compiler/linker
  # settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(googletest concurrentqueue slogpp)
  enable_testing()

  include(GoogleTest)

  add_custom_target(check ALL ${CMAKE_CTEST_COMMAND} ARGS --output-on-failure)
else()
  FetchContent_MakeAvailable(concurrentqueue slogpp)
endif()

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype REQUIRED)

add_subdirectory(src/gl)
