cmake_minimum_required(VERSION 3.10)
project(ATF3_0 C)

set(CMAKE_C_STANDARD 11)

find_package(CURL REQUIRED)

find_path(JSON_C_INCLUDE_DIR
        NAMES json-c/json.h
        PATHS /usr/local/include)

find_library(JSON_C_LIBRARY
        NAMES json-c
        PATHS /usr/local/lib)

include_directories(${JSON_C_INCLUDE_DIR})

add_executable(ATF3_0 main.c)

target_link_libraries(ATF3_0 PRIVATE CURL::libcurl ${JSON_C_LIBRARY})
