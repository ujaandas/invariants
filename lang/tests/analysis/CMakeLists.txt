add_executable(analysis_test dependency_graph_test.cpp)

target_link_libraries(analysis_test PRIVATE invariants_analysis GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(analysis_test)
