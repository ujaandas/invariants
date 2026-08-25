add_library(invariants_dependency dependency.cpp)

target_include_directories(invariants_dependency PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_dependency
    PUBLIC invariants_ast
)