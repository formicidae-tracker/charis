include(CheckCSourceCompiles)

check_c_source_compiles(
	"#include <time.h>
int main() {
  struct timespec wall;
  clock_gettime(CLOCK_REALTIME,&wall);
  return 0;
}"
	NEED_NO_RT_LINK
)
if(NOT NEED_NO_RT_LINK)
	set(CMAKE_REQUIRED_LIBRARIES "-lrt")
	check_c_source_compiles(
		"#include <time.h>
int main() {
  struct timespec wall;
  clock_gettime(CLOCK_REALTIME,&wall);
  return 0;
}"
		NEED_RT_LINK
	)
	if(NOT NEED_RT_LINK)
		message(FATAL_ERROR "Could not clock_gettime()")
	endif(NOT NEED_RT_LINK)
endif(NOT NEED_NO_RT_LINK)

find_package(Protobuf 3.3.0 REQUIRED)

set(FORT_TIME_VERSION 1.0.11)
set(FORT_TIME_ABI 1)

configure_file(version.hpp.in version.hpp @ONLY)

add_library(fort-time SHARED Time.cpp Time.hpp)
add_library(fort-charis::libfort-time ALIAS fort-time)
set_target_properties(
	fort-time PROPERTIES VERSION ${FORT_TIME_VERSION} #
						 SOVERSION ${FORT_TIME_ABI}
)
target_include_directories(
	fort-time INTERFACE ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src
)

target_link_libraries(fort-time PUBLIC protobuf::libprotobuf)

if(NEED_RT_LINK)
	target_link_libraries(fort-time PUBLIC "-lrt")
endif(NEED_RT_LINK)

if(NOT CHARIS_IMPORTED)
	add_executable(fort-time-tests TimeTest.cpp)
	target_link_libraries(fort-time-tests fort-time GTest::gtest_main)

	gtest_discover_tests(fort-time-tests)
	add_dependencies(check fort-time-tests)
endif(NOT CHARIS_IMPORTED)

install(FILES Time.hpp ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
		DESTINATION include/fort/time
)
install(TARGETS fort-time DESTINATION lib)
