if(FORT_CHARIS_BUILD_TIME)
	add_subdirectory(time)
endif(FORT_CHARIS_BUILD_TIME)

FetchContent_Declare(
	concurrentqueue
	GIT_REPOSITORY https://github.com/cameron314/concurrentqueue.git
	GIT_TAG v1.0.4
)
FetchContent_MakeAvailable(concurrentqueue)
if(FORT_CHARIS_USE_MAGIC_ENUM)
	FetchContent_Declare(
		magic_enum
		GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
		GIT_TAG v0.9.7
	)
	FetchContent_MakeAvailable(magic_enum)
endif(FORT_CHARIS_USE_MAGIC_ENUM)

if(FORT_CHARIS_BUILD_UTILS)
	add_subdirectory(utils)
endif(FORT_CHARIS_BUILD_UTILS)

if(FORT_CHARIS_BUILD_GL)

	FetchContent_Declare(
		slogpp
		GIT_REPOSITORY https://github.com/atuleu/slogpp.git
		GIT_TAG 525bd84d12e4b16916344da83fa73646d7deb957
	)

	FetchContent_MakeAvailable(slogpp)

	find_package(glfw3 REQUIRED)
	find_package(GLEW REQUIRED)
	find_package(OpenGL REQUIRED)
	find_package(Eigen3 REQUIRED)
	find_package(Fontconfig REQUIRED)
	find_package(Freetype REQUIRED)

	add_subdirectory(gl)
endif(FORT_CHARIS_BUILD_GL)

if(FORT_CHARIS_BUILD_VIDEO)
	set(CPPTRACE_BUILD_SHARED
		On
		CACHE BOOL "Build shared lib" FORCE
	)
	FetchContent_Declare(
		cpptrace
		GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
		GIT_TAG v0.3.1 # <HASH or TAG>
	)
	FetchContent_Declare(
		spng
		GIT_REPOSITORY https://github.com/randy408/libspng.git
		GIT_TAG v0.7.4
	)

	FetchContent_MakeAvailable(cpptrace spng)

	find_package(PkgConfig REQUIRED)

	pkg_check_modules(
		ffmpeg
		REQUIRED
		libavutil
		libavcodec
		libavformat
		libswscale
		IMPORTED_TARGET
	)

	add_subdirectory(video)

endif(FORT_CHARIS_BUILD_VIDEO)

if(FORT_CHARIS_BUILD_OPTIONS)

	add_subdirectory(options)

endif(FORT_CHARIS_BUILD_OPTIONS)
