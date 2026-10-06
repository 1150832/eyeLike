# EyeTracker 3D: Modular & Distributed 3D Eye-Tracking System

[![System Demonstration Video](https://img.youtube.com/vi/XuiCPCdbaDI/maxresdefault.jpg)](https://www.youtube.com/watch?v=XuiCPCdbaDI)

> 📺 **Demonstração Experimental / Video Showcase:** [https://www.youtube.com/watch?v=XuiCPCdbaDI](https://www.youtube.com/watch?v=XuiCPCdbaDI)

An OpenCV-based computer vision framework originally based on Fabian Timm's image gradient eye center localization algorithm, significantly re-engineered and extended into a **modular, low-cost distributed 3D eye-tracking architecture**.

---

### 🎓 Academic Context
This repository forms the experimental framework developed as part of an **MSc Thesis in Electrical and Computer Engineering** at the **Instituto Superior de Engenharia do Porto (ISEP)**.

The primary research objective is the development of a low-cost, distributed 3D tracking architecture. Edge computing nodes (e.g., Raspberry Pi 4 running via `libcamera`/GStreamer) and desktop nodes execute monocular 2D facial and sub-pixel pupil tracking, publishing standardized telemetry via **MQTT**. A centralized spatial fusion node ingests these asynchronous streams, synchronizing frames and reconstructing real-time 3D coordinates (X, Y, Z) using **Direct Linear Transformation (DLT)** and **Singular Value Decomposition (SVD)**.

---

## 📽️ Demonstration Breakdown

A complete recorded demonstration of the experimental validation pipeline is available on YouTube:
* **Direct Link:** https://www.youtube.com/watch?v=XuiCPCdbaDI

### Video Index:
1. **Ground Truth Annotation Tool (`annotationTool`)**: Custom annotation interface with real-time gamma, contrast, and histogram equalization controls, enabling sub-pixel pupil labeling and occlusion flag logging.
2. **Quantitative Benchmarking (`testEyeTracking`)**: Algorithmic evaluation against annotated datasets, reporting real-time Euclidean error metrics and blink/occlusion handling.
3. **Monocular 2D Tracking & MQTT Telemetry**: Real-time pupil localization and sub-pixel gradient extraction integrated with Mosquitto broker telemetry streaming.
4. **Distributed Stereo Architecture & 3D Spatial Fusion**: Heterogeneous edge-host synchronization (Raspberry Pi 4 + PC) executing temporal sliding-window pairing and 3D stereo triangulation.

---

## 🧩 System Architecture & Modules

The repository is structured into four core modules:

* **`eyeLike` (Core Tracking Node)**: Implements Viola-Jones Haar Cascade face detection and Timm & Barth gradient-based eye center localization. Features an integrated asynchronous Paho MQTT C++ publisher emitting structured JSON telemetry (`eyetracker/coordinates/{sensor_id}/eyes`).
* **`annotationTool` (Ground Truth Annotation)**: Interactive tool developed to curate reference datasets, supporting dynamic brightness/contrast adaptation and frame status toggling.
* **`testEyeTracking` (Accuracy Validation)**: Comparative testbench calculating per-frame pixel error (L2 distance) between algorithmic predictions and annotated ground truth.
* **`tools/fuse_3d_mqtt.py` (Central 3D Fusion Engine)**: Python-based subscriber utilizing a temporal sliding-window queue to align telemetry packets from distributed nodes and solve the stereo DLT/SVD system (`cv2.triangulatePoints`), recording spatial data to CSV.

---

## ⚙️ Building & Execution

CMake is required across all platforms.

### 1. Raspberry Pi 4 (Raspberry Pi OS 64-bit)

Running on embedded Linux requires interfacing with the modern `libcamera` stack and headless execution:

```bash
# Clone and build
mkdir build && cd build
cmake ..
make -j4

# Run with libcamerify wrapper (headless MQTT node)
QT_QPA_PLATFORM=offscreen libcamerify ./eyelike --headless -c -m tcp://<BROKER_IP>:1883 --client-id rpi_node
```

Alternatively, invoke via GStreamer pipeline:
```bash
QT_QPA_PLATFORM=offscreen ./eyelike -v "libcamerasrc ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! appsink" -m tcp://<BROKER_IP>:1883 --client-id rpi_node
```

### 2. Windows (MSVC + vcpkg)

Requires Visual Studio C++ Build Tools, CMake, and local OpenCV 4.x.

**Prerequisites & Dependencies:**
1. Extract **OpenCV 4.x** to `C:\opencv`. Ensure `C:\opencv\build\x64\vc16\bin` is added to your system `Path`.
2. Install `vcpkg` and the Eclipse Paho MQTT C++ library:
```powershell
cd ~
git clone [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg install paho-mqttpp3:x64-windows
```

**Compilation:**
Run the dedicated automated build script:
```powershell
.\scripts\win\cmakeBuild.bat
```
Binaries (`eyelike.exe`, `annotationTool.exe`, `testEyeTracking.exe`) will be generated inside `build\Release\`.

### 3. Linux / macOS

```bash
mkdir build && cd build
cmake ..
make
./bin/eyeLike
```

---

## 📡 3D Telemetry Fusion Engine

To execute the central 3D spatial triangulation node:

```bash
# Install Python dependencies
pip install paho-mqtt numpy opencv-python

# Run the fusion engine
python tools/fuse_3d_mqtt.py
```
Triangulated spatial vectors (X, Y, Z) and network delta timestamps (Delta T) are logged continuously to `test_reports/telemetry/3d_telemetry_log.csv`.

---

## 📚 References & Academic Attribution

* **Timm and Barth (2011)**: Accurate eye centre localisation by means of gradients. In Proceedings of the Int. Conference on Computer Vision Theory and Applications (VISAPP), volume 1, pages 125-130, Algarve, Portugal. INSTICC.
* **Original eyeLike Implementation**: Tristan Hume (http://thume.ca/projects/2012/11/04/simple-accurate-eye-center-tracking-in-opencv/).
* **MSc Dissertation**: Low-cost Modular 3D Eye-Tracking System, Departamento de Engenharia Eletrotécnica, Instituto Superior de Engenharia do Porto (ISEP), 2026.
