add_executable(lexer_token_test token_test.cpp)
add_executable(lexer_test lexer_test.cpp)

target_link_libraries(lexer_token_test PRIVATE invariants_lexer GTest::gtest_main)
target_link_libraries(lexer_test PRIVATE invariants_lexer GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(lexer_token_test)
gtest_discover_tests(lexer_test)
