add_library(invariants_ir INTERFACE)

target_include_directories(invariants_ir INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_ir
    INTERFACE invariants_ast
)