add_library(invariants_binder symbol_table.cpp binder.cpp)

target_include_directories(invariants_binder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_binder
    PUBLIC invariants_ast
)