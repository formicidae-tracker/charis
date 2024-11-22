set(TEST_SRC_FILES #
	OptionsTest.cpp #
	details/OptionTest.cpp #
	details/TraitsTest.cpp
)

add_library(fort-options INTERFACE)
add_library(fort-charis::libfort-options ALIAS fort-options)
target_include_directories(fort-options INTERFACE ${PROJECT_SOURCE_DIR}/src)

if(NOT CHARIS_IMPORTED)

	add_executable(charis-options-tests ${TEST_SRC_FILES} ${HDR_FILES})
	target_link_libraries(
		charis-options-tests GTest::gtest_main fort-charis::libfort-options
	)
	gtest_discover_tests(charis-options-tests)
	add_dependencies(check charis-options-tests)
endif(NOT CHARIS_IMPORTED)

install(FILES ${HDR_FILES} DESTINATION include/fort/options)
