cmake_minimum_required(VERSION 3.10)
PROJECT(eyelike_project)

FIND_PACKAGE(OpenCV REQUIRED)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(testing)
