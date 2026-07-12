add_executable(ir_test ir_dummy_test.cpp)

target_link_libraries(ir_test
    PRIVATE invariants_ir
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ir_test)
