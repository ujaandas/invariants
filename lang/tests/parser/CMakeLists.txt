add_executable(parser_test parser_bop_test.cpp parser_expr_tests.cpp parser_type_tests.cpp)

target_link_libraries(parser_test
    PRIVATE invariants_parser invariants_lexer
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(parser_test)
