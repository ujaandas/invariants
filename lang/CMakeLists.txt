cmake_minimum_required(VERSION 3.25)

project(invariants)

enable_testing()

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(GTest REQUIRED)

add_executable(hello_world src/main.cpp)

install(TARGETS hello_world RUNTIME DESTINATION bin)

add_subdirectory(tests)