add_library(invariants_env env.cpp)

target_include_directories(invariants_env PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_env
    PUBLIC invariants_ast invariants_lexer
)