add_library(invariants_resolver resolver.cpp)

target_include_directories(invariants_resolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_resolver
    PUBLIC invariants_ast
)