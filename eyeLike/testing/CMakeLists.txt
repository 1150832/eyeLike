cmake_minimum_required(VERSION 3.5)
project(EyeTrackingTestTools)

# Find OpenCV
find_package(OpenCV REQUIRED)

# Set C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include OpenCV directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Annotation Tool
add_executable(annotationTool annotationTool.cpp)
target_link_libraries(annotationTool ${OpenCV_LIBS})

# Testing Tool
add_executable(testEyeTracking testEyeTracking.cpp)
target_link_libraries(testEyeTracking ${OpenCV_LIBS})

message(STATUS "Eye tracking testing tools configured")
