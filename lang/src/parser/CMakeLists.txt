add_library(invariants_parser parser.cpp)

target_include_directories(invariants_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(invariants_parser
    PUBLIC invariants_lexer invariants_ast
)