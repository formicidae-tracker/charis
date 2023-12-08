set(SRC_FILES Defer.cpp)
set(HDR_FILES Defer.hpp)

set(TEST_SRC_FILES DeferTest.cpp)
set(TEST_HDR_FILES DeferTest.hpp)

if(NOT CHARIS_IMPORTED)
  add_executable(charis-utils-tests ${TEST_SRC_FILES} ${TEST_HDR_FILES})
  target_link_libraries(charis-utils-tests GTest::gtest_main)
  gtest_discover_tests(charis-utils-tests)
  add_dependencies(check charis-utils-tests)
else(NOT CHARIS_IMPORTED)
  add_library(fort-charis::libfort-utils INTERFACE IMPORTED GLOBAL)
  target_include_directories(fort-charis::libfort-utils
                             INTERFACE ${PROJECT_SOURCE_DIR}/src)

endif(NOT CHARIS_IMPORTED)
