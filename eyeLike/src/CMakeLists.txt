# eyeLike executable with MQTT support

# Set C++17 standard (required for MQTT library)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add all source files including MQTT
ADD_EXECUTABLE(eyeLike 
    main.cpp 
    findEyeCenter.cpp 
    findEyeCenter.h 
    findEyeCorner.cpp 
    findEyeCorner.h 
    constants.h 
    helpers.cpp 
    helpers.h
    MqttPublisher.cpp
    MqttPublisher.h
)

# Explicitly set MacPorts MQTT library paths
set(PAHO_MQTT_CPP "/opt/local/lib/libpaho-mqttpp3.dylib")
set(PAHO_MQTT_C "/opt/local/lib/libpaho-mqtt3as.dylib")

# Include directories - MacPorts paths
include_directories(/opt/local/include)

# Link OpenCV (required)
TARGET_LINK_LIBRARIES(eyeLike ${OpenCV_LIBS})

# Link MQTT libraries
if(EXISTS ${PAHO_MQTT_CPP} AND EXISTS ${PAHO_MQTT_C})
    TARGET_LINK_LIBRARIES(eyeLike ${PAHO_MQTT_CPP} ${PAHO_MQTT_C})
    message(STATUS "✓ MQTT support enabled (C++17)")
    message(STATUS "  PAHO C++: ${PAHO_MQTT_CPP}")
    message(STATUS "  PAHO C:   ${PAHO_MQTT_C}")
else()
    message(FATAL_ERROR "MQTT libraries not found in /opt/local/lib")
endif()
