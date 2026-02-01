cmake_minimum_required(VERSION 3.5)
project(eyelike_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

# Paths for MQTT (MacPorts)
set(PAHO_MQTT_CPP "/opt/local/lib/libpaho-mqttpp3.dylib")
set(PAHO_MQTT_C "/opt/local/lib/libpaho-mqtt3as.dylib")
include_directories(/opt/local/include)
include_directories(src)

# Main Executable
add_executable(eyeLike 
    src/main.cpp 
    src/findEyeCenter.cpp 
    src/findEyeCorner.cpp 
    src/helpers.cpp
    src/MqttPublisher.cpp
)
target_link_libraries(eyeLike ${OpenCV_LIBS})
if(EXISTS ${PAHO_MQTT_CPP})
    target_link_libraries(eyeLike ${PAHO_MQTT_CPP} ${PAHO_MQTT_C})
endif()

# --- Testing Tools ---
# Build the Annotation Tool
add_executable(annotationTool testing/src/annotationTool.cpp)
target_link_libraries(annotationTool ${OpenCV_LIBS})

# Build the Eye Tracking Test Tool
add_executable(testEyeTracking testing/src/testEyeTracking.cpp)
target_link_libraries(testEyeTracking ${OpenCV_LIBS})