# eyeLike - Eye Tracking System

A modular, low-cost 3D eye tracking system using Raspberry Pi 5 (also works on macOS/Linux) with real-time MQTT publishing and comprehensive testing framework.

## Features

- ✅ Real-time eye tracking using OpenCV and Haar Cascades
- ✅ Video and webcam support
- ✅ MQTT publishing for real-time coordinate streaming
- ✅ Ground truth annotation tool for quality testing
- ✅ Automated testing framework with detailed metrics
- ✅ CSV export for data analysis
- ✅ Cross-platform support (macOS, Linux, Raspberry Pi)

## Quick Start
```bash
# Clone the repository
git clone <your-repo-url>
cd eyeLike

# Build
./cmakeBuild.sh

# Run with webcam
build/bin/eyeLike -c

# Run with video file
build/bin/eyeLike -v path/to/video.mp4 -o output.csv

# Run with MQTT publishing
build/bin/eyeLike -v video.mp4 -m tcp://localhost:1883
```

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)
- [Testing Framework](#testing-framework)
- [MQTT Integration](#mqtt-integration)
- [Documentation](#documentation)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)

## Installation

See [INSTALLATION.md](docs/INSTALLATION.md) for detailed setup instructions.

### Prerequisites

- CMake 3.10+
- OpenCV 4.x
- C++17 compiler
- MQTT libraries (optional, for MQTT support)

### macOS (Catalina and newer)
```bash
# Using MacPorts
sudo port install opencv4
sudo port install mosquitto paho.mqtt.c paho.mqtt.cpp

# Build
./cmakeBuild.sh
```

### Linux / Raspberry Pi
```bash
# Install dependencies
sudo apt-get install cmake libopencv-dev
sudo apt-get install mosquitto libpaho-mqtt-dev libpaho-mqttpp-dev

# Build
./cmakeBuild.sh
```

## Usage

### Basic Eye Tracking
```bash
# Webcam
build/bin/eyeLike -c

# Video file
build/bin/eyeLike -v input.mp4 -o results.csv

# Save specific frame
# Press 'f' during execution to save current frame
```

### With MQTT Publishing
```bash
# Start MQTT broker
mosquitto -v

# Run eyeLike with MQTT
build/bin/eyeLike -v video.mp4 -m tcp://localhost:1883

# Subscribe to see data
mosquitto_sub -h localhost -t "eyetracker/coordinates" -v
```

### Command-Line Options
```
Usage: eyeLike [options]

Options:
  -c, --camera              Use webcam
  -v, --video <path>        Process video file
  -o, --output <path>       Save results to CSV
  -m, --mqtt <broker>       Enable MQTT (default: tcp://localhost:1883)
  -t, --topic <topic>       MQTT topic (default: eyetracker/coordinates)
  --client-id <id>          MQTT client ID (default: eyeLike)
  -h, --help                Show help

Keyboard Controls:
  'c' or 'q'  - Quit
  'f'         - Save current frame
  'p'         - Pause video (video mode only)
```

## Testing Framework

eyeLike includes a complete testing framework for quality assurance. See [TESTING.md](docs/TESTING.md) for details.

### Quick Testing Workflow
```bash
# 1. Create ground truth annotations
./annotate.sh test_data/video.mp4

# 2. Generate predictions
build/bin/eyeLike -v test_data/video.mp4 -o test_data/predictions.csv

# 3. Run quality tests
./runTest.sh test_data/video.mp4 \
             test_data/ground_truth_video.json \
             test_data/predictions.csv

# Or use the all-in-one pipeline
./run_complete_test.sh test_data/video.mp4
```

### Testing Tools

- **annotationTool** - Manual ground truth annotation with GUI
- **testEyeTracking** - Automated quality metrics and visualization
- **Complete pipeline scripts** - End-to-end testing automation

## MQTT Integration

Real-time eye coordinate streaming via MQTT. See [MQTT.md](docs/MQTT.md) for details.

### MQTT Message Format
```json
{
  "frame": 15,
  "timestamp": 0.500,
  "face": {"x": 100, "y": 120, "width": 200, "height": 200},
  "left_eye": {"x": 150, "y": 180},
  "right_eye": {"x": 250, "y": 180}
}
```

## Documentation

- [Installation Guide](docs/INSTALLATION.md) - Detailed setup for all platforms
- [Testing Framework](docs/TESTING.md) - Complete testing workflow
- [MQTT Integration](docs/MQTT.md) - Real-time data streaming
- [API Reference](docs/API.md) - Code documentation

## Project Structure
```
eyeLike/
├── src/                    # Source code
│   ├── main.cpp           # Main application
│   ├── findEyeCenter.cpp  # Eye detection algorithm
│   ├── MqttPublisher.cpp  # MQTT integration
│   └── ...
├── testing/               # Testing framework
│   ├── annotationTool.cpp # Ground truth annotation
│   └── testEyeTracking.cpp # Quality testing
├── test_data/            # Test videos and annotations
├── test_reports/         # Test results
├── res/                  # Resources (Haar cascades)
├── docs/                 # Documentation
└── build/                # Build output
```

## Output Formats

### CSV Format
```csv
frame,face_x,face_y,face_width,face_height,right_eye_x,right_eye_y,left_eye_x,left_eye_y,...
0,100,120,200,200,250,180,150,180,...
```

### Ground Truth JSON Format
```json
{
  "video_metadata": {
    "filename": "test.mp4",
    "fps": 30,
    "resolution": [1920, 1080]
  },
  "annotations": [
    {
      "frame_number": 0,
      "timestamp": 0.0,
      "left_eye": {"x": 150, "y": 180},
      "right_eye": {"x": 250, "y": 180}
    }
  ]
}
```

## Performance

Typical performance on different platforms:

- **macOS (2012 MacBook Pro)**: ~15-20 FPS (720p)
- **Raspberry Pi 5**: ~20-30 FPS (720p)
- **Modern Desktop**: ~30+ FPS (1080p)

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## Troubleshooting

### Common Issues

**Build fails with MQTT errors:**
```bash
# Make sure MQTT libraries are installed
sudo port install paho.mqtt.cpp  # macOS
sudo apt-get install libpaho-mqttpp-dev  # Linux
```

**Camera not detected:**
```bash
# Check camera permissions (macOS)
# System Preferences → Security & Privacy → Camera

# Linux - check device
ls /dev/video*
```

**Low frame rate:**
- Reduce video resolution
- Adjust `kSmoothFaceFactor` in `src/constants.h`
- Use faster hardware

## Acknowledgments

- Original eyeLike project
- OpenCV community
- Eclipse Paho MQTT

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Master's Thesis Project

This is part of a Master's thesis in Electrical and Computer Engineering at ISEP (Instituto Superior de Engenharia do Porto), focused on developing a modular, low-cost 3D eye tracking system.

**Thesis Title:** EyeTracker 3D Modular  
**Institution:** ISEP-DEE  
**Program:** Mestrado em Engenharia Eletrotécnica e de Computadores  
**Year:** 2024/25

## Contact

For questions or support, please open an issue on GitHub.

---

**Made with ❤️ for the TEDI Project**
