add_library(invariants_runtime evaluator.cpp runtime.cpp)

target_include_directories(invariants_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_runtime
    PUBLIC invariants_ast invariants_binder invariants_analysis
)