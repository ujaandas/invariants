add_executable(binder_test symbol_table_test.cpp)

target_link_libraries(binder_test PRIVATE invariants_binder invariants_parser invariants_lexer GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(binder_test)
