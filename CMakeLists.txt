cmake_minimum_required(VERSION 3.10)

project(WiFiGeolocation)

# Find necessary packages
find_package(CURL REQUIRED)
find_path(JSON-C_INCLUDE_DIR json-c/json.h)
find_library(JSON-C_LIBRARY NAMES json-c)

# Add the source files to the project
add_executable(ATF3_0 main.c)

# Include the necessary directories
target_include_directories(ATF3_0 PRIVATE ${CURL_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIR})

# Link the necessary libraries
target_link_libraries(ATF3_0 PRIVATE ${CURL_LIBRARIES} ${JSON-C_LIBRARY})
