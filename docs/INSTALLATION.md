# Installation Guide

Complete installation instructions for eyeLike eye tracking system.

## Table of Contents

- [System Requirements](#system-requirements)
- [macOS Installation](#macos-installation)
- [Linux Installation](#linux-installation)
- [Raspberry Pi Installation](#raspberry-pi-installation)
- [Verifying Installation](#verifying-installation)
- [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

- **CPU**: Dual-core processor (1.5 GHz+)
- **RAM**: 2 GB
- **Storage**: 500 MB for application + space for videos
- **Camera**: USB webcam or built-in camera (optional)

### Software Requirements

- **CMake**: 3.10 or higher
- **C++ Compiler**: Supporting C++17
  - macOS: Xcode Command Line Tools / Clang
  - Linux: GCC 7+ or Clang 5+
- **OpenCV**: 4.0 or higher
- **MQTT Libraries** (optional): Paho MQTT C and C++

## macOS Installation

### Option 1: MacPorts (Recommended for macOS Catalina and older)
```bash
# 1. Install Xcode Command Line Tools
xcode-select --install

# 2. Install MacPorts from https://www.macports.org/install.php

# 3. Update MacPorts
sudo port selfupdate

# 4. Install dependencies
sudo port install cmake
sudo port install opencv4

# 5. Install MQTT (optional)
sudo port install mosquitto
sudo port install paho.mqtt.c
sudo port install paho.mqtt.cpp

# 6. Clone and build
git clone <your-repo-url>
cd eyeLike
./cmakeBuild.sh
```

### Option 2: Homebrew (macOS Big Sur and newer)
```bash
# 1. Install Homebrew from https://brew.sh

# 2. Install dependencies
brew install cmake
brew install opencv

# 3. Install MQTT (optional)
brew install mosquitto
brew install paho-mqtt-c
brew install paho-mqtt-cpp

# 4. Clone and build
git clone <your-repo-url>
cd eyeLike
./cmakeBuild.sh
```

## Linux Installation

### Ubuntu / Debian
```bash
# 1. Update package manager
sudo apt-get update

# 2. Install build tools
sudo apt-get install build-essential cmake git

# 3. Install OpenCV
sudo apt-get install libopencv-dev

# 4. Install MQTT (optional)
sudo apt-get install mosquitto mosquitto-clients
sudo apt-get install libpaho-mqtt-dev libpaho-mqttpp-dev

# 5. Clone and build
git clone <your-repo-url>
cd eyeLike
./cmakeBuild.sh
```

### Fedora / RHEL
```bash
# Install dependencies
sudo dnf install cmake gcc-c++ opencv-devel
sudo dnf install mosquitto paho-c-devel paho-cpp-devel

# Clone and build
git clone <your-repo-url>
cd eyeLike
./cmakeBuild.sh
```

## Raspberry Pi Installation

### Raspberry Pi OS (Debian-based)
```bash
# 1. Update system
sudo apt-get update
sudo apt-get upgrade

# 2. Install dependencies
sudo apt-get install build-essential cmake git
sudo apt-get install libopencv-dev

# 3. Install MQTT (optional)
sudo apt-get install mosquitto mosquitto-clients
sudo apt-get install libpaho-mqtt-dev libpaho-mqttpp-dev

# 4. Clone and build
git clone <your-repo-url>
cd eyeLike
./cmakeBuild.sh

# Note: First build may take 10-20 minutes on Raspberry Pi
```

### Raspberry Pi-Specific Optimizations

For better performance on Raspberry Pi:
```bash
# Enable hardware acceleration
sudo raspi-config
# Navigate to: Advanced Options → GL Driver → GL (Full KMS)

# Adjust constants for Pi performance
# Edit src/constants.h and reduce kSmoothFaceFactor
```

## Building from Source

### Standard Build
```bash
cd eyeLike
./cmakeBuild.sh
```

### Manual Build (if script fails)
```bash
cd eyeLike
mkdir -p build
cd build
cmake ..
make -j4
```

### Build Options
```bash
# Clean build
rm -rf build/*
./cmakeBuild.sh

# Build with verbose output
cd build
cmake ..
make VERBOSE=1

# Build testing tools only
cd testing
mkdir -p build && cd build
cmake ..
make
```

## Verifying Installation

### Test eyeLike
```bash
# Show help
build/bin/eyeLike --help

# Test with webcam (press 'q' to quit)
build/bin/eyeLike -c

# Test without camera (will show usage)
build/bin/eyeLike
```

### Test Testing Tools
```bash
# Should show usage
testing/build/annotationTool
testing/build/testEyeTracking
```

### Test MQTT (if installed)
```bash
# Start broker
mosquitto -v

# In another terminal, test connection
mosquitto_pub -h localhost -t "test" -m "hello"
mosquitto_sub -h localhost -t "test"
```

## Troubleshooting

### CMake Can't Find OpenCV
```bash
# Check OpenCV installation
pkg-config --modversion opencv4
# or
pkg-config --modversion opencv

# If not found, reinstall
# macOS:
sudo port install opencv4
# Linux:
sudo apt-get install libopencv-dev
```

### C++17 Compiler Errors
```bash
# Check compiler version
g++ --version  # Should be 7.0+
clang++ --version  # Should be 5.0+

# Update if needed
# Ubuntu:
sudo apt-get install g++-9
# Then edit CMakeLists.txt to use g++-9
```

### MQTT Libraries Not Found
```bash
# Check if installed
ls /opt/local/lib/libpaho*  # macOS MacPorts
ls /opt/homebrew/lib/libpaho*  # macOS Homebrew
ls /usr/lib/libpaho*  # Linux

# If not found, install
# macOS:
sudo port install paho.mqtt.cpp
# Linux:
sudo apt-get install libpaho-mqttpp-dev
```

### Camera Permission Denied (macOS)
```bash
# Grant camera access
# System Preferences → Security & Privacy → Camera
# Check the box next to Terminal or your IDE
```

### Camera Not Detected (Linux)
```bash
# Check camera devices
ls /dev/video*

# Test with v4l2
sudo apt-get install v4l-utils
v4l2-ctl --list-devices

# Add user to video group
sudo usermod -a -G video $USER
# Then logout and login again
```

### Build Fails on Raspberry Pi
```bash
# May need to increase swap size
sudo nano /etc/dphys-swapfile
# Change CONF_SWAPSIZE=100 to CONF_SWAPSIZE=1024
sudo /etc/init.d/dphys-swapfile restart

# Then rebuild
```

## Updating
```bash
# Pull latest changes
git pull

# Rebuild
rm -rf build/*
./cmakeBuild.sh
```

## Uninstalling
```bash
# Remove build artifacts
rm -rf build/
rm -rf testing/build/

# Remove dependencies (optional)
# macOS:
sudo port uninstall opencv4 paho.mqtt.cpp
# Linux:
sudo apt-get remove libopencv-dev libpaho-mqttpp-dev
```

## Next Steps

After successful installation:

1. Read [README.md](../README.md) for usage overview
2. See [TESTING.md](TESTING.md) for testing framework
3. Check [MQTT.md](MQTT.md) for MQTT integration
4. Try the quick start examples in README.md

## Getting Help

If you encounter issues:

1. Check this troubleshooting section
2. Search existing GitHub issues
3. Create a new issue with:
   - Your OS and version
   - Error messages
   - Steps to reproduce

---

**Installation complete? Start with: `build/bin/eyeLike --help`**
