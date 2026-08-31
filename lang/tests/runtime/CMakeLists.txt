add_executable(runtime_test runtime_test.cpp)

target_link_libraries(runtime_test PRIVATE invariants_runtime GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(runtime_test)
