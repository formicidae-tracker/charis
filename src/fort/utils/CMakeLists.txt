option(FORT_CHARIS_CAPITALIZE_DEFER Off)

set(SRC_FILES Defer.cpp Once.cpp ObjectPool.cpp LRUCache.cpp)
set(HDR_FILES Defer.hpp Once.hpp ObjectPool.hpp LRUCache.hpp)

set(TEST_SRC_FILES DeferTest.cpp OnceTest.cpp ObjectPoolTest.cpp
				   LRUCacheTest.cpp
)

add_library(fort-utils INTERFACE)
add_library(fort-charis::libfort-utils ALIAS fort-utils)
target_include_directories(fort-utils INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(fort-utils INTERFACE concurrentqueue)

if(FORT_CHARIS_CAPITALIZE_DEFER)
	target_compile_definitions(
		fort-utils INTERFACE -DFORT_CHARIS_CAPITALIZE_DEFER=1
	)
endif(FORT_CHARIS_CAPITALIZE_DEFER)

if(NOT CHARIS_IMPORTED)
	add_executable(charis-utils-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
	target_link_libraries(
		charis-utils-tests GTest::gtest_main concurrentqueue
		fort-charis::libfort-utils
	)
	gtest_discover_tests(charis-utils-tests)
	add_dependencies(check charis-utils-tests)
endif(NOT CHARIS_IMPORTED)

install(FILES ${HDR_FILES} DESTINATION include/fort/utils)
