add_executable(resolver_test symbol_table_test.cpp resolver_ast_test.cpp resolver_test.cpp)

target_link_libraries(resolver_test PRIVATE invariants_resolver invariants_parser invariants_lexer GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(resolver_test)
