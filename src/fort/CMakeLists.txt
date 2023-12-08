if(FORT_CHARIS_BUILD_UTILS)
  add_subdirectory(utils)
endif(FORT_CHARIS_BUILD_UTILS)

if(FORT_CHARIS_BUILD_GL)

  FetchContent_Declare(
    slogpp
    GIT_REPOSITORY https://github.com/atuleu/slogpp.git
    GIT_TAG 525bd84d12e4b16916344da83fa73646d7deb957)

  FetchContent_Declare(
    concurrentqueue
    GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
    GIT_TAG v1.0.4)

  FetchContent_MakeAvailable(concurrentqueue slogpp)

  find_package(glfw3 REQUIRED)
  find_package(GLEW REQUIRED)
  find_package(OpenGL REQUIRED)
  find_package(Eigen3 REQUIRED)
  find_package(Fontconfig REQUIRED)
  find_package(Freetype REQUIRED)

  add_subdirectory(gl)
endif(FORT_CHARIS_BUILD_GL)
