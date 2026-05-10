cmake_minimum_required(VERSION 3.25)

project(invariants)


# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable compile commands for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(INVARIANTS_BUILD_TESTS "Build tests" ON)

add_subdirectory(src)

if(INVARIANTS_BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()

add_executable(hello_world src/main.cpp)

target_link_libraries(hello_world PRIVATE invariants_lexer)

install(TARGETS hello_world RUNTIME DESTINATION bin)
