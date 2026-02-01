#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <string>

// Forward declaration
namespace mqtt {
    class async_client;
}

// MQTT publishing modes
enum class MqttMode {
    PRODUCTION,    // Separate topics, optimized messages
    DEBUG,         // Complete JSON payloads
    HEARTBEAT      // Minimal, only on changes
};

class MqttPublisher {
private:
    mqtt::async_client* client;
    std::string baseTopic;
    std::string sensorId;
    std::string macAddress;
    bool connected;
    bool enabled;
    MqttMode mode;
    
    // Last values for change detection (heartbeat mode)
    int lastFaceX, lastFaceY, lastFaceW, lastFaceH;
    int lastLeftX, lastLeftY, lastRightX, lastRightY;
    bool lastFaceDetected;
    
    // Helper methods
    std::string getMacAddress();
    void publishToTopic(const std::string& subtopic, const std::string& payload, int qos = 0);
    bool hasSignificantChange(int newFaceX, int newFaceY, int newFaceW, int newFaceH,
                               int newLeftX, int newLeftY, int newRightX, int newRightY);

public:
    MqttPublisher();
    ~MqttPublisher();
    
    // Initialize MQTT connection with Last Will and Testament
    bool connect(const std::string& broker, 
                 const std::string& clientId, 
                 const std::string& baseTopic,
                 MqttMode publishMode = MqttMode::PRODUCTION);
    
    // Publish eye coordinates (adapts based on mode)
    void publishEyeData(int frame, int faceX, int faceY, int faceW, int faceH,
                        int rightEyeX, int rightEyeY, int leftEyeX, int leftEyeY,
                        bool faceDetected = true);
    
    // Publish heartbeat (keep-alive)
    void publishHeartbeat();
    
    // Publish sensor discovery info
    void publishDiscovery();
    
    // Check if connected
    bool isConnected() const { return connected && enabled; }
    
    // Get sensor ID
    std::string getSensorId() const { return sensorId; }
    
    // Change mode at runtime
    void setMode(MqttMode newMode) { mode = newMode; }
    
    // Disconnect
    void disconnect();
};

#endif // MQTT_PUBLISHER_H
