#include "MqttPublisher.h"
#include <mqtt/async_client.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>

#ifdef __APPLE__
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#elif __linux__
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#endif

MqttPublisher::MqttPublisher() 
    : client(nullptr), connected(false), enabled(false), mode(MqttMode::PRODUCTION),
      lastFaceX(0), lastFaceY(0), lastFaceW(0), lastFaceH(0),
      lastLeftX(0), lastLeftY(0), lastRightX(0), lastRightY(0),
      lastFaceDetected(false) {
    macAddress = getMacAddress();
    sensorId = "sensor_" + macAddress;
}

MqttPublisher::~MqttPublisher() {
    disconnect();
}

std::string MqttPublisher::getMacAddress() {
    std::string mac = "unknown";
    
#ifdef __APPLE__
    int mib[6];
    size_t len;
    char *buf;
    unsigned char *ptr;
    struct if_msghdr *ifm;
    struct sockaddr_dl *sdl;
    
    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    
    if ((mib[5] = if_nametoindex("en0")) == 0) {
        return mac;
    }
    
    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
        return mac;
    }
    
    if ((buf = (char*)malloc(len)) == NULL) {
        return mac;
    }
    
    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        free(buf);
        return mac;
    }
    
    ifm = (struct if_msghdr *)buf;
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (unsigned char *)LLADDR(sdl);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 6; i++) {
        // Removed colon for clean topic usage
        ss << std::setw(2) << (int)ptr[i];
    }
    mac = ss.str();
    free(buf);
    
#elif __linux__
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
    if (sock == -1) {
        return mac;
    }
    
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
        close(sock);
        return mac;
    }
    
    struct ifreq* it = ifc.ifc_req;
    const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
    
    for (; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    unsigned char* ptr = (unsigned char*)ifr.ifr_hwaddr.sa_data;
                    std::stringstream ss;
                    ss << std::hex << std::setfill('0');
                    for (int i = 0; i < 6; i++) {
                        // Removed colon for clean topic usage
                        ss << std::setw(2) << (int)ptr[i];
                    }
                    mac = ss.str();
                    break;
                }
            }
        }
    }
    close(sock);
#endif
    
    return mac;
}

bool MqttPublisher::connect(const std::string& broker, const std::string& clientId, 
                             const std::string& baseTopicParam, MqttMode publishMode) {
    try {
        // Use simplified topic structure: base/sensor_ID
        baseTopic = baseTopicParam + "/" + sensorId;
        mode = publishMode;
        
        client = new mqtt::async_client(broker, clientId + "_" + macAddress);
        
        // Configure Last Will and Testament (LWT)
        std::string lwt_topic = baseTopic + "/status";
        mqtt::will_options willOpts(lwt_topic, std::string("offline"), 1, true);

        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        connOpts.set_will(willOpts); 
                
        std::cout << "╔═══════════════════════════════════════════════════╗\n";
        std::cout << "║         MQTT Sensor Connection                   ║\n";
        std::cout << "╚═══════════════════════════════════════════════════╝\n";
        std::cout << "  Broker:     " << broker << "\n";
        std::cout << "  Topic:      " << baseTopic << "\n";
        std::cout << "  Mode:       ";
        
        switch(mode) {
            case MqttMode::PRODUCTION:  std::cout << "Production (QoS 0, Optimized)\n"; break;
            case MqttMode::DEBUG:       std::cout << "Debug (QoS 0, Full JSON)\n"; break;
            case MqttMode::HEARTBEAT:   std::cout << "Heartbeat (QoS 1, Retained, Changes Only)\n"; break;
        }
        
        std::cout << "\nConnecting...\n";
        client->connect(connOpts)->wait();
        
        connected = true;
        enabled = true;
        
        // Publish online status
        publishToTopic("/status", "online", 1, true);
        
        // Publish discovery information
        publishDiscovery();
        
        std::cout << "✓ Connected successfully!\n";
        std::cout << "══════════════════════════════════════════════════════\n\n";
        
        return true;
    } catch (const mqtt::exception& e) {
        std::cerr << "✗ MQTT connection failed: " << e.what() << std::endl;
        connected = false;
        enabled = false;
        return false;
    }
}

// Updated helper with 'retained' parameter
void MqttPublisher::publishToTopic(const std::string& subtopic, const std::string& payload, int qos, bool retained) {
    if (!isConnected()) return;
    
    try {
        std::string fullTopic = baseTopic + subtopic;
        mqtt::message_ptr msg = mqtt::make_message(fullTopic, payload);
        msg->set_qos(qos);
        msg->set_retained(retained); // Set retention
        client->publish(msg);
    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT publish error: " << e.what() << std::endl;
    }
}

bool MqttPublisher::hasSignificantChange(int newFaceX, int newFaceY, int newFaceW, int newFaceH,
                                          int newLeftX, int newLeftY, int newRightX, int newRightY) {
    // Threshold: 5 pixels for position, 10 pixels for size
    int posThreshold = 5;
    int sizeThreshold = 10;
    
    return (std::abs(newFaceX - lastFaceX) > posThreshold ||
            std::abs(newFaceY - lastFaceY) > posThreshold ||
            std::abs(newFaceW - lastFaceW) > sizeThreshold ||
            std::abs(newFaceH - lastFaceH) > sizeThreshold ||
            std::abs(newLeftX - lastLeftX) > posThreshold ||
            std::abs(newLeftY - lastLeftY) > posThreshold ||
            std::abs(newRightX - lastRightX) > posThreshold ||
            std::abs(newRightY - lastRightY) > posThreshold);
}

void MqttPublisher::publishEyeData(int frame, int faceX, int faceY, int faceW, int faceH,
                                    int rightEyeX, int rightEyeY, int leftEyeX, int leftEyeY,
                                    bool faceDetected) {
    if (!isConnected()) return;
    
    // Determine QoS and Retain based on mode
    int qos = 0;
    bool retain = false;
    
    if (mode == MqttMode::HEARTBEAT) {
        qos = 1;        // Ensure delivery for state changes
        retain = true;  // Keep last known state on broker
        
        // Filter changes
        if (faceDetected == lastFaceDetected && 
            !hasSignificantChange(faceX, faceY, faceW, faceH, leftEyeX, leftEyeY, rightEyeX, rightEyeY)) {
            return; // No significant change, skip publishing
        }
    }
    
    // Update last values
    lastFaceX = faceX; lastFaceY = faceY;
    lastFaceW = faceW; lastFaceH = faceH;
    lastLeftX = leftEyeX; lastLeftY = leftEyeY;
    lastRightX = rightEyeX; lastRightY = rightEyeY;
    lastFaceDetected = faceDetected;
    
    try {
        double timestamp = frame / 30.0;
        
        if (mode == MqttMode::DEBUG) {
            // Debug mode: Full JSON with MAC/ID included (as requested)
            std::ostringstream json;
            json << "{"
                 << "\"sensor_id\":\"" << sensorId << "\","
                 << "\"mac\":\"" << macAddress << "\","
                 << "\"frame\":" << frame << ","
                 << "\"timestamp\":" << std::fixed << std::setprecision(3) << timestamp << ","
                 << "\"face\":{\"detected\":" << (faceDetected ? "true" : "false")
                 << ",\"x\":" << faceX << ",\"y\":" << faceY 
                 << ",\"width\":" << faceW << ",\"height\":" << faceH << "},"
                 << "\"left_eye\":{\"x\":" << leftEyeX << ",\"y\":" << leftEyeY << "},"
                 << "\"right_eye\":{\"x\":" << rightEyeX << ",\"y\":" << rightEyeY << "},"
                 << "\"eye_center\":{\"x\":" << (leftEyeX + rightEyeX)/2 
                 << ",\"y\":" << (leftEyeY + rightEyeY)/2 << "}"
                 << "}";
            
            publishToTopic("/debug/raw", json.str(), 0, false);
            
        } else {
            // Production & Heartbeat: Optimized JSON (No MAC/ID in payload)
            
            // 1. Topic: .../face
            std::ostringstream faceJson;
            faceJson << "{"
                     << "\"frame\":" << frame << ","
                     << "\"timestamp\":" << std::fixed << std::setprecision(3) << timestamp << ","
                     << "\"detected\":" << (faceDetected ? "true" : "false");
            
            if (faceDetected) {
                faceJson << ",\"x\":" << faceX << ",\"y\":" << faceY 
                         << ",\"w\":" << faceW << ",\"h\":" << faceH;
            }
            faceJson << "}";
            
            // Publish Face with specific QoS/Retain
            publishToTopic("/face", faceJson.str(), qos, retain);
            
            // 2. Topic: .../eyes (Only if face detected)
            if (faceDetected) {
                std::ostringstream eyesJson;
                eyesJson << "{"
                         << "\"frame\":" << frame << ","
                         << "\"timestamp\":" << std::fixed << std::setprecision(3) << timestamp << ","
                         << "\"left\":{\"x\":" << leftEyeX << ",\"y\":" << leftEyeY << "},"
                         << "\"right\":{\"x\":" << rightEyeX << ",\"y\":" << rightEyeY << "},"
                         << "\"center\":{\"x\":" << (leftEyeX + rightEyeX)/2 
                         << ",\"y\":" << (leftEyeY + rightEyeY)/2 << "}"
                         << "}";
                         
                publishToTopic("/eyes", eyesJson.str(), qos, retain);
            }
        }
        
    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT publish error: " << e.what() << std::endl;
    }
}

void MqttPublisher::publishHeartbeat() {
    if (!isConnected()) return;
    
    // Heartbeat status message (always QoS 1, Retained)
    std::ostringstream payload;
    payload << "{\"status\":\"alive\",\"timestamp\":" << std::time(nullptr) << "}";
    publishToTopic("/heartbeat", payload.str(), 1, true);
}

void MqttPublisher::publishDiscovery() {
    if (!isConnected()) return;
    
    std::ostringstream discovery;
    discovery << "{"
              << "\"sensor_id\":\"" << sensorId << "\","
              << "\"mac\":\"" << macAddress << "\","
              << "\"type\":\"eye_tracker\","
              << "\"version\":\"1.2.0\","
              << "\"topics\":[\"face\",\"eyes\"],"
              << "\"modes\":[\"production\",\"heartbeat\",\"debug\"]"
              << "}";
    
    size_t lastSlash = baseTopic.find_last_of('/');
    std::string discoveryTopic = (lastSlash != std::string::npos) 
        ? baseTopic.substr(0, lastSlash) + "/discovery"
        : "eyetracker/discovery";

    mqtt::message_ptr msg = mqtt::make_message(discoveryTopic, discovery.str());
    msg->set_qos(1);
    msg->set_retained(true);
    client->publish(msg);
}

void MqttPublisher::disconnect() {
    if (client && connected) {
        try {
            // Publish offline status
            publishToTopic("/status", "offline", 1, true);
            
            std::cout << "\nDisconnecting from MQTT broker...\n";
            client->disconnect()->wait();
            delete client;
            client = nullptr;
            connected = false;
        } catch (const mqtt::exception& e) {
            std::cerr << "MQTT disconnect error: " << e.what() << std::endl;
        }
    }
}
