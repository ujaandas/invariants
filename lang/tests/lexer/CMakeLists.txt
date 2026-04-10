add_executable(lexer_token_test token_test.cpp)

target_link_libraries(lexer_token_test PRIVATE invariants_lexer GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lexer_token_test)
