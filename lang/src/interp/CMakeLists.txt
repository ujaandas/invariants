add_library(invariants_interp interpreter.cpp)

target_include_directories(invariants_interp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_interp
    PUBLIC invariants_lexer
)