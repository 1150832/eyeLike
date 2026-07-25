## eyeLike
An OpenCV based webcam gaze tracker based on a simple image gradient-based eye center algorithm by Fabian Timm.

*Note: This repository is currently being expanded as part of an MSc Thesis in Electrical and Computer Engineering at Instituto Superior de Engenharia do Porto (ISEP). The goal is the development of a modular, low-cost 3D EyeTracker system leveraging the eyelike functionality running in a modular architecture, such as a Raspberry Pi, using Image Analysis and Machine Learning. The tracking data is standardized and published via MQTT, allowing the module to work standalone for safety monitoring or in a cluster for 3D positioning evaluation.*

## DISCLAIMER
**This does not track gaze yet.** It is basically just a developer reference implementation of Fabian Timm's algorithm that shows some debugging windows with points on your pupils.

If you want cheap gaze tracking and don't mind hardware check out [The Eye Tribe](https://theeyetribe.com/).
If you want webcam-based eye tracking contact [Xlabs](http://xlabsgaze.com/) or use their chrome plugin and SDK.
If you're looking for open source your only real bet is [Pupil](http://pupil-labs.com/) but that requires an expensive hardware headset.

## Status
The eye center tracking works well but I don't have a reference point like eye corner yet so it can't actually track
where the user is looking.

Current version detects both eyes to improve 3d head tracking. Eyes position is presented in global image coordinates.


## Building

CMake is required to build eyeLike.

### OSX or Linux with Make
```bash
# do things in the build directory so that we don't clog up the main directory
mkdir build
cd build
cmake ../
make
./bin/eyeLike # the executable file
```

### On OSX with XCode
```bash
mkdir build
./cmakeBuild.sh
```
then open the XCode project in the build folder and run from there.

### On Windows (MSVC)

The Windows environment requires a local installation of OpenCV and the use of `vcpkg` for the MQTT networking dependencies.

**1. Prerequisites & Dependencies:**
* Ensure Visual Studio C++ Build Tools and CMake are installed.
* Extract **OpenCV 4.x** to `C:\opencv`. 
  * *Critical:* You must add `C:\opencv\build\x64\vc16\bin` to your Windows `Path` Environment Variable so the system can locate the `.dll` files at runtime.
* Install **vcpkg** and the Eclipse Paho MQTT C++ wrapper:
  ```powershell
  cd ~
  git clone [https://github.com/microsoft/vcpkg.git](https://github.com/microsoft/vcpkg.git)
  cd vcpkg
  .\bootstrap-vcpkg.bat
  .\vcpkg install paho-mqttpp3:x64-windows

**2. Compilation:**
* Adjust the VCPKG_ROOT path inside scripts\win\cmakeBuild.bat if your vcpkg is not installed in the default user directory.
Then, run:
  ```powershell
  .\scripts\win\cmakeBuild.bat
* The compiled binaries (eyelike.exe, annotationTool.exe, and testEyeTracking.exe) will be generated inside the build\Release folder, alongside the automatically linked MQTT .dll files.

## Blog Article:
- [Using Fabian Timm's Algorithm](http://thume.ca/projects/2012/11/04/simple-accurate-eye-center-tracking-in-opencv/)

## Paper:
Timm and Barth. Accurate eye centre localisation by means of gradients.
In Proceedings of the Int. Conference on Computer Theory and
Applications (VISAPP), volume 1, pages 125-130, Algarve, Portugal,
2011. INSTICC.

(also see youtube video at http://www.youtube.com/watch?feature=player_embedded&v=aGmGyFLQAFM)
