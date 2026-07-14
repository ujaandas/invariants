add_executable(symbol_table_test symbol_table_test.cpp)

target_link_libraries(symbol_table_test PRIVATE invariants_resolver GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(symbol_table_test)
