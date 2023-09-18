cmake_minimum_required(VERSION 3.10)
project(ATF)

set(CMAKE_C_STANDARD 99)

find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)

add_executable(ATF google_geolocation.c)

target_include_directories(ATF PRIVATE ${JANSSON_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_libraries(ATF PRIVATE ${JANSSON_LIBRARIES} ${CURL_LIBRARIES})
