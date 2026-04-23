add_executable(interpreter_test interpreter_test.cpp)

target_link_libraries(interpreter_test
    PRIVATE invariants_interp GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(interpreter_test)
