cmake_minimum_required(VERSION 3.10)
project(eyelike_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PahoMqttCpp CONFIG REQUIRED)  

include_directories(src)

if(WIN32)
    # Windows cfg (via vcpkg)
    find_package(PahoMqttCpp CONFIG REQUIRED)
    find_package(eclipse-paho-mqtt-c CONFIG REQUIRED)
elseif(APPLE)
    # macOS cfg (via MacPorts) 
    set(PAHO_MQTT_CPP "/opt/local/lib/libpaho-mqttpp3.dylib")
    set(PAHO_MQTT_C "/opt/local/lib/libpaho-mqtt3as.dylib")
    include_directories(/opt/local/include)
endif()

# ==========================================
# MAIN EXECUTABLE
# ==========================================
add_executable(eyelike 
    src/main.cpp 
    src/findEyeCenter.cpp 
    src/findEyeCorner.cpp 
    src/helpers.cpp
    src/MqttPublisher.cpp
)

if(WIN32)
    target_link_libraries(eyelike PRIVATE iphlpapi ws2_32)
endif()

target_link_libraries(eyelike PRIVATE
    ${OpenCV_LIBS}
    PahoMqttCpp::paho-mqttpp3
)
# ==========================================
# TESTING TOOLS
# ==========================================
# Build the Annotation Tool
add_executable(annotationTool testing/src/annotationTool.cpp)
target_link_libraries(annotationTool ${OpenCV_LIBS})

# Build the Eye Tracking Test Tool
add_executable(testEyeTracking testing/src/testEyeTracking.cpp)
target_link_libraries(testEyeTracking ${OpenCV_LIBS})