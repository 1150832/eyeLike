# Eye Tracking Testing Framework for TEDI Project

A comprehensive testing framework for evaluating eye tracking quality using ground truth annotations.

## Overview

This framework consists of two C++ tools:

1. **annotationTool** - Interactive GUI for manually annotating eye positions in videos
2. **testEyeTracking** - Automated testing tool that compares eyeLike predictions against ground truth

## Features

### Annotation Tool
- ✅ Fast OpenCV-based GUI
- ✅ Frame-by-frame navigation
- ✅ Visual markers for left/right eye positions
- ✅ Jump to next unannotated frame
- ✅ Export/import JSON annotations
- ✅ Auto-save prompts
- ✅ Keyboard shortcuts for efficiency

### Testing Tool
- ✅ Automatic error metrics calculation
- ✅ Visual comparison overlay
- ✅ Per-frame error visualization
- ✅ Statistical analysis (mean, std dev, max)
- ✅ Accuracy threshold reporting
- ✅ Detailed CSV report generation

## Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake
sudo apt-get install libopencv-dev

# macOS (with Homebrew)
brew install opencv

# Fedora/RHEL
sudo dnf install opencv-devel
```

### Build

```bash
# Clone or navigate to your TEDI project directory
cd path/to/TEDI

# Place the tool files in your project
# - annotationTool.cpp
# - testEyeTracking.cpp
# - Makefile

# Build both tools
make

# Or build individually
make annotation  # Only annotation tool
make test       # Only testing tool

# Optional: Install system-wide
sudo make install
```

## Usage

### Step 1: Create Ground Truth Annotations

```bash
./annotationTool test_video.mp4
```

**Workflow:**
1. Video loads in window
2. Click on **LEFT eye** (blue marker appears)
3. Click on **RIGHT eye** (green marker appears)
4. Navigate to next frame (arrow keys or buttons)
5. Repeat for desired frames (every 10-30 frames recommended)
6. Press **'S'** to save annotations

**Keyboard Controls:**
```
Navigation:
  →, D          Next frame
  ←, A          Previous frame
  Page Down     +10 frames
  Page Up       -10 frames
  Home          +30 frames
  End           -30 frames
  N             Jump to next unannotated
  P             Jump to previous unannotated

Annotation:
  Left Click    Mark eye position
  C, Delete     Clear current frame
  S             Save to JSON
  L             Load existing JSON
  
Other:
  H, ?          Show help
  Q, ESC        Quit (with save prompt)
```

**Output:** `ground_truth_test_video.json`

### Step 2: Generate Predictions with eyeLike

```bash
# Run your existing eyeLike code on the same video
./eyeLike -v test_video.mp4 -o predictions.csv
```

This generates a CSV file with columns:
```
frame,face_x,face_y,face_width,face_height,right_eye_x,right_eye_y,left_eye_x,left_eye_y,...
```

### Step 3: Run Quality Tests

```bash
./testEyeTracking test_video.mp4 ground_truth_test_video.json predictions.csv
```

**What it does:**
1. Loads ground truth and predictions
2. Calculates error metrics automatically
3. Displays results in terminal
4. Generates detailed report: `test_report.txt`
5. Opens visualization window for frame-by-frame inspection

**Keyboard Controls (Test Mode):**
```
  →, D          Next frame
  ←, A          Previous frame
  Page Down     +10 frames
  Page Up       -10 frames
  Q, ESC        Quit
```

**Visualization Legend:**
- **Blue solid circle** = Ground truth LEFT eye
- **Green solid circle** = Ground truth RIGHT eye
- **Red outline** = Predicted LEFT eye
- **Yellow outline** = Predicted RIGHT eye
- **Lines** = Error distance with pixel measurement

## Output Files

### Ground Truth JSON Format
```json
{
  "video_metadata": {
    "filename": "test_video.mp4",
    "fps": 30,
    "resolution": [1920, 1080],
    "total_frames": 900
  },
  "annotations": [
    {
      "frame_number": 0,
      "timestamp": 0.0,
      "left_eye": {"x": 450, "y": 320},
      "right_eye": {"x": 550, "y": 318}
    },
    ...
  ]
}
```

### Test Report Format
```
EYE TRACKING TEST REPORT
========================

Dataset Overview:
  Total annotated frames:    50
  Frames with predictions:   48
  Detection rate:            96.0%

Error Statistics (pixels):
  Left Eye  - Avg: 8.45 +/- 4.23, Max: 18.76
  Right Eye - Avg: 7.89 +/- 3.91, Max: 16.54
  Overall   - Avg: 8.17 px

Accuracy Thresholds:
  Within  5px: 18 / 48 (37.5%)
  Within 10px: 38 / 48 (79.2%)
  Within 20px: 47 / 48 (97.9%)
  Within 50px: 48 / 48 (100.0%)

Per-Frame Detailed Results:
Frame,Left_Error_px,Right_Error_px,Avg_Error_px
0,7.21,6.32,6.77
15,9.85,8.12,8.99
...
```

## Annotation Strategy

### Recommended Approach
1. **Keyframe sampling**: Annotate every 10-30 frames (not every single frame)
2. **Focus on variety**: Different head poses, lighting, distances
3. **Challenging scenarios**: Extreme angles, occlusions, motion blur
4. **Minimum dataset**: 50-100 annotated frames for statistical significance
5. **Quality over quantity**: Accurate annotations > many annotations

### Tips for Quality Annotations
- Click at the **center of the pupil** (not iris edge)
- Zoom in (resize window) for precision
- Be consistent with your definition of "eye center"
- Mark LEFT eye first (matches your code convention)
- Take breaks to maintain concentration

## Integration with Existing Code

### Your Current Workflow
```bash
# Original: Just run eyeLike
./eyeLike -v video.mp4 -o output.csv

# New: Test quality first
./annotationTool video.mp4              # Create ground truth
./eyeLike -v video.mp4 -o predictions.csv
./testEyeTracking video.mp4 ground_truth_video.json predictions.csv
```

### Automated Testing Script

Create `run_tests.sh`:
```bash
#!/bin/bash

VIDEO=$1
GROUND_TRUTH=$2

if [ -z "$VIDEO" ] || [ -z "$GROUND_TRUTH" ]; then
    echo "Usage: $0 <video_file> <ground_truth.json>"
    exit 1
fi

echo "Running eyeLike on $VIDEO..."
./eyeLike -v "$VIDEO" -o predictions_temp.csv

echo "Running quality tests..."
./testEyeTracking "$VIDEO" "$GROUND_TRUTH" predictions_temp.csv

echo "Test complete! Check test_report.txt for results."
```

## Metrics Explained

### Euclidean Distance Error
```
error = sqrt((x_pred - x_true)² + (y_pred - y_true)²)
```
The pixel distance between predicted and ground truth eye positions.

### Average Error
Mean of all per-frame errors. Lower is better.
- **< 5px**: Excellent accuracy
- **5-10px**: Good accuracy
- **10-20px**: Acceptable for most applications
- **> 20px**: Needs improvement

### Standard Deviation
Measures consistency. Low std dev = reliable predictions.

### Detection Rate
Percentage of annotated frames where eyeLike successfully detected eyes.
- **> 95%**: Robust detection
- **85-95%**: Good detection
- **< 85%**: May need parameter tuning

### Accuracy Thresholds
Percentage of predictions within specific pixel distances:
- **Within 10px** is a common benchmark for eye tracking applications
- **Within 20px** is acceptable for gaze estimation
- **Within 50px** indicates general detection success

## Troubleshooting

### Annotation Tool Issues

**Video won't load:**
```bash
# Check OpenCV video codec support
opencv_version --verbose

# Try converting video to a compatible format
ffmpeg -i input.mp4 -c:v libx264 -preset fast output.mp4
./annotationTool output.mp4
```

**Window is too small/large:**
- The window is resizable - just drag the corners
- Or edit the code's `CV_WINDOW_NORMAL` flag

**Annotations not saving:**
- Check write permissions in current directory
- Try specifying full path: `./annotationTool /full/path/video.mp4`

### Testing Tool Issues

**CSV parsing errors:**
- Ensure CSV matches expected format (check header row)
- Your eyeLike must output: `frame,face_x,face_y,face_width,face_height,right_eye_x,right_eye_y,left_eye_x,left_eye_y,...`

**Frame mismatch warnings:**
- Ground truth and predictions may have different frame numbers
- This is normal - tool only compares frames present in both datasets

## File Structure

```
TEDI/
├── main.cpp                 # Your existing eyeLike code
├── annotationTool.cpp       # New: Annotation tool
├── testEyeTracking.cpp      # New: Testing tool
├── Makefile                 # Updated build system
├── res/                     # Haar cascades, etc.
├── test_data/              # Testing videos and annotations
│   ├── video1.mp4
│   ├── ground_truth_video1.json
│   └── predictions_video1.csv
└── test_reports/           # Test results
    └── report_video1.txt
```

## Advanced Usage

### Batch Testing Multiple Videos

Create `batch_test.sh`:
```bash
#!/bin/bash

for video in test_data/*.mp4; do
    basename=$(basename "$video" .mp4)
    gt="test_data/ground_truth_${basename}.json"
    
    if [ -f "$gt" ]; then
        echo "Testing $basename..."
        ./eyeLike -v "$video" -o "predictions_${basename}.csv"
        ./testEyeTracking "$video" "$gt" "predictions_${basename}.csv"
        mv test_report.txt "test_reports/report_${basename}.txt"
    fi
done

echo "Batch testing complete!"
```

### Continuous Integration

Add to your development workflow:
```bash
# Before committing changes to eyeLike
make clean && make
./run_tests.sh test_video.mp4 ground_truth.json

# Only commit if accuracy hasn't regressed
```

## Performance Optimization

### For Annotation
- Annotate keyframes only (every N frames)
- Use keyboard shortcuts instead of mouse navigation
- Process multiple short videos instead of one long video

### For Testing
- Keep ground truth datasets small but representative
- Run tests on the same hardware as production
- Cache results for regression testing

## Contributing Test Data

If you create high-quality annotated datasets:
1. Include diverse scenarios (lighting, angles, distances)
2. Document the annotation process used
3. Include both "easy" and "challenging" frames
4. Provide video metadata (fps, resolution, conditions)

## Future Enhancements

Planned features:
- [ ] Multi-person annotation support
- [ ] Confidence score tracking
- [ ] Heatmap visualization
- [ ] Automated keyframe selection
- [ ] Inter-annotator agreement metrics
- [ ] Real-time annotation during video playback

## FAQ

**Q: How many frames should I annotate?**
A: 50-100 frames gives good statistical significance. Focus on diverse scenarios.

**Q: Should I annotate every frame?**
A: No! Keyframe sampling (every 10-30 frames) is more efficient and equally valid.

**Q: What if eyeLike fails to detect eyes in some frames?**
A: Normal! The detection rate metric tracks this. Aim for > 90% detection.

**Q: Can I use this with other eye tracking software?**
A: Yes! Just export to the same CSV format that eyeLike uses.

**Q: How do I handle partially occluded eyes?**
A: Annotate the estimated center position, or skip the frame if too ambiguous.

## License

Part of the TEDI project. Same license as eyeLike.

## Contact

For issues or questions about the testing framework, refer to the main TEDI project documentation.

---

**Happy Testing! 🎯👁️**