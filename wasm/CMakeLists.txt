cmake_minimum_required(VERSION 3.25)

project(invariants_wasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(../lang/src/lexer lexer)

add_executable(invariants_wasm
  lib.cpp
)

target_link_libraries(invariants_wasm PRIVATE
  invariants_lexer
)

if(EMSCRIPTEN)
  target_link_options(invariants_wasm PRIVATE
    "--bind"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
  )
endif()

file(COPY
  index.html
  main.js
  styles.css
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)