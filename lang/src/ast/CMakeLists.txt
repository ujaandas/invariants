add_library(invariants_ast visitors/printer.cpp visitors/tree_printer.cpp)

target_include_directories(invariants_ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})