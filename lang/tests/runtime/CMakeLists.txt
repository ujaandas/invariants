add_executable(runtime_test runtime_test.cpp evaluator_test.cpp evaluator_op_test.cpp)

target_link_libraries(runtime_test PRIVATE invariants_runtime invariants_lexer invariants_parser invariants_binder GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(runtime_test)
