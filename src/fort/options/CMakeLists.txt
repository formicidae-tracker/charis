set(TEST_SRC_FILES #
	OptionsTest.cpp #
	details/OptionTest.cpp #
	details/TraitsTest.cpp #
	details/DesignatorTest.cpp
	details/ArgsLexerTest.cpp
)

set(HDR_FILES
	Options.hpp #
	details/Option.hpp #
	details/Traits.hpp #
	details/config.hpp
	details/Designator.hpp
	details/ArgsLexer.hpp
)

add_library(fort-options INTERFACE)
add_library(fort-charis::libfort-options ALIAS fort-options)
target_include_directories(fort-options INTERFACE ${PROJECT_SOURCE_DIR}/src)

if(FORT_CHARIS_USE_MAGIC_ENUM)
	target_link_libraries(fort-options INTERFACE magic_enum::magic_enum)
endif(FORT_CHARIS_USE_MAGIC_ENUM)

if(FORT_CHARIS_USE_FKYAML)
	target_link_libraries(fort-options INTERFACE fkYAML::fkYAML)
endif(FORT_CHARIS_USE_FKYAML)

if(NOT CHARIS_IMPORTED)

	add_executable(charis-options-tests ${TEST_SRC_FILES} ${HDR_FILES})
	target_link_libraries(
		charis-options-tests GTest::gtest_main GTest::gmock
		fort-charis::libfort-options cpptrace::cpptrace
	)
	gtest_discover_tests(charis-options-tests)
	add_dependencies(check charis-options-tests)
endif(NOT CHARIS_IMPORTED)

install(FILES ${HDR_FILES} DESTINATION include/fort/options)
