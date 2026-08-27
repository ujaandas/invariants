add_library(invariants_analysis dependency_graph.cpp)

target_include_directories(invariants_analysis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_analysis
    PUBLIC invariants_ast invariants_binder
)