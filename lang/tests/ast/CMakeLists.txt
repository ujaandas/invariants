add_executable(ast_test ast_node_test.cpp ast_print_node_test.cpp ast_eq_test.cpp)

target_link_libraries(ast_test
    PRIVATE invariants_ast 
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ast_test)
