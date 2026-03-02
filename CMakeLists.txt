cmake_minimum_required(VERSION 3.20)
project(pane VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The engine library
add_library(pane_engine
    src/main.c
)

target_include_directories(pane_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
