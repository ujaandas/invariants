cmake_minimum_required(VERSION 3.25)

project(invariants_wasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(INVARIANTS_BUILD_TESTS OFF)

add_subdirectory(../lang lang_build)

add_executable(invariants_wasm
  lib.cpp
)

target_link_libraries(invariants_wasm PRIVATE
  invariants_parser
)

if(EMSCRIPTEN)
  target_compile_options(invariants_wasm PRIVATE
    "-fexceptions"
  )

  target_link_options(invariants_wasm PRIVATE
    "--bind"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-fexceptions"
    "-sASSERTIONS=2"
    "-sSAFE_HEAP=1"
    "-sSTACK_OVERFLOW_CHECK=2"
  )
endif()

file(COPY
  index.html
  main.js
  styles.css
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)