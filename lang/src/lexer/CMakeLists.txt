add_library(invariants_lexer token.cpp lexer.cpp)

target_include_directories(invariants_lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
