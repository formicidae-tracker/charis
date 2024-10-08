find_package(Python REQUIRED)

add_custom_target(
	charis-options-pytests
	${Python_EXECUTABLE} -m pytest
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python
)

add_dependencies(check charis-options-pytests)

set(SRC_FILES details/Traits.cpp details/Option.cpp Options.cpp)
set(HDR_FILES details/Traits.hpp details/Option.cpp Options.hpp)

set(TEST_SRC_FILES details/TraitsTest.cpp details/OptionTest.cpp
				   OptionsTest.cpp
)

add_library(fort-charis::libfort-options INTERFACE IMPORTED GLOBAL)
target_include_directories(
	fort-charis::libfort-options INTERFACE ${PROJECT_SOURCE_DIR}/src
)

if(NOT CHARIS_IMPORTED)
	add_executable(charis-options-tests ${TEST_SRC_FILES} ${HDR_FILES})
	target_link_libraries(
		charis-options-tests GTest::gtest_main fort-charis::libfort-options
	)
	gtest_discover_tests(charis-options-tests)
	add_dependencies(check charis-options-tests)
endif(NOT CHARIS_IMPORTED)
