add_executable(parser_test parser_test.cpp parser_bop_test.cpp)

target_link_libraries(parser_test
    PRIVATE invariants_parser invariants_lexer
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(parser_test)
