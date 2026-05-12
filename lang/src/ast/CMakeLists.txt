add_library(invariants_ast printers/plain_printer.cpp printers/tree_printer.cpp)

target_include_directories(invariants_ast PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})