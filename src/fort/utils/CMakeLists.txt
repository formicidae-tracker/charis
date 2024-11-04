set(SRC_FILES Defer.cpp Once.cpp ObjectPool.cpp)
set(HDR_FILES Defer.hpp Once.hpp ObjectPool.hpp)

set(TEST_SRC_FILES DeferTest.cpp OnceTest.cpp ObjectPoolTest.cpp)

if(NOT CHARIS_IMPORTED)
	add_executable(charis-utils-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
	target_link_libraries(charis-utils-tests GTest::gtest_main concurrentqueue)
	gtest_discover_tests(charis-utils-tests)
	add_dependencies(check charis-utils-tests)
endif(NOT CHARIS_IMPORTED)

add_library(fort-charis::libfort-utils INTERFACE IMPORTED GLOBAL)
target_include_directories(
	fort-charis::libfort-utils INTERFACE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(fort-charis::libfort-utils INTERFACE concurrentqueue)

install(FILES ${HDR_FILES} DESTINATION include/fort/utils)
