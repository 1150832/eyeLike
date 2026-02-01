#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <sstream>

/* Support for OpenCV 4.x */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#define CV_WINDOW_AUTOSIZE cv::WINDOW_AUTOSIZE
#endif

// Structure to hold eye annotation data
struct EyeAnnotation {
    cv::Point left_eye;
    cv::Point right_eye;
    bool left_set;
    bool right_set;
    
    EyeAnnotation() : left_eye(-1, -1), right_eye(-1, -1), left_set(false), right_set(false) {}
    
    bool isComplete() const {
        return left_set && right_set;
    }
    
    void clear() {
        left_eye = cv::Point(-1, -1);
        right_eye = cv::Point(-1, -1);
        left_set = false;
        right_set = false;
    }
};

// Global variables
std::map<int, EyeAnnotation> annotations;
int currentFrame = 0;
cv::Mat currentFrameImage;
cv::Mat displayImage;
cv::VideoCapture videoCapture;
std::string videoFilename;
int totalFrames = 0;
double fps = 30.0;
bool isDirty = false; // Track if annotations have been modified

// Function declarations
void drawAnnotations(cv::Mat& image, int frameNum);
void mouseCallback(int event, int x, int y, int flags, void* userdata);
void displayHelp();
void saveAnnotations(const std::string& filename);
bool loadAnnotations(const std::string& filename);
void goToFrame(int frameNum);
int findNextUnannotated(int startFrame, bool forward);
void updateDisplay();
std::string getJsonFilename(const std::string& videoPath);

// Mouse callback function
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event != cv::EVENT_LBUTTONDOWN) {
        return;
    }
    
    EyeAnnotation& ann = annotations[currentFrame];
    
    if (!ann.left_set) {
        ann.left_eye = cv::Point(x, y);
        ann.left_set = true;
        std::cout << "Frame " << currentFrame << " - Left eye set at: (" << x << ", " << y << ")\n";
    } else if (!ann.right_set) {
        ann.right_eye = cv::Point(x, y);
        ann.right_set = true;
        std::cout << "Frame " << currentFrame << " - Right eye set at: (" << x << ", " << y << ")\n";
    } else {
        // Both already set, reset and start over
        ann.clear();
        ann.left_eye = cv::Point(x, y);
        ann.left_set = true;
        std::cout << "Frame " << currentFrame << " - Reset and set left eye at: (" << x << ", " << y << ")\n";
    }
    
    isDirty = true;
    updateDisplay();
}

// Draw annotations on the image
void drawAnnotations(cv::Mat& image, int frameNum) {
    if (annotations.find(frameNum) == annotations.end()) {
        return;
    }
    
    const EyeAnnotation& ann = annotations[frameNum];
    
    // Draw left eye (blue)
    if (ann.left_set) {
        cv::circle(image, ann.left_eye, 8, cv::Scalar(255, 100, 0), -1);  // Filled circle
        cv::circle(image, ann.left_eye, 10, cv::Scalar(255, 150, 0), 2);  // Outline
        cv::putText(image, "L", cv::Point(ann.left_eye.x - 5, ann.left_eye.y + 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
    
    // Draw right eye (green)
    if (ann.right_set) {
        cv::circle(image, ann.right_eye, 8, cv::Scalar(0, 255, 100), -1);  // Filled circle
        cv::circle(image, ann.right_eye, 10, cv::Scalar(0, 255, 150), 2);  // Outline
        cv::putText(image, "R", cv::Point(ann.right_eye.x - 5, ann.right_eye.y + 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
}

// Update the display with current frame and annotations
void updateDisplay() {
    if (currentFrameImage.empty()) {
        return;
    }
    
    currentFrameImage.copyTo(displayImage);
    drawAnnotations(displayImage, currentFrame);
    
    // Draw overlay with info
    cv::Mat overlay;
    displayImage.copyTo(overlay);
    
    // Semi-transparent background for text
    cv::rectangle(overlay, cv::Point(10, 10), cv::Point(400, 120), 
                  cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(overlay, 0.7, displayImage, 0.3, 0, displayImage);
    
    // Frame info
    std::stringstream ss;
    ss << "Frame: " << currentFrame << " / " << (totalFrames - 1);
    cv::putText(displayImage, ss.str(), cv::Point(20, 35),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    // Annotation status
    int annotatedCount = 0;
    for (const auto& pair : annotations) {
        if (pair.second.isComplete()) {
            annotatedCount++;
        }
    }
    
    ss.str("");
    ss << "Annotated: " << annotatedCount << " frames";
    cv::putText(displayImage, ss.str(), cv::Point(20, 65),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200, 200, 200), 1);
    
    // Current frame status
    const EyeAnnotation& ann = annotations[currentFrame];
    std::string status;
    cv::Scalar statusColor;
    
    if (ann.isComplete()) {
        status = "Status: COMPLETE";
        statusColor = cv::Scalar(0, 255, 0);
    } else if (ann.left_set) {
        status = "Status: Click RIGHT eye";
        statusColor = cv::Scalar(0, 255, 255);
    } else {
        status = "Status: Click LEFT eye";
        statusColor = cv::Scalar(255, 100, 0);
    }
    
    cv::putText(displayImage, status, cv::Point(20, 95),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, statusColor, 2);
    
    cv::imshow("Eye Annotation Tool", displayImage);
}

// Go to specific frame
void goToFrame(int frameNum) {
    if (frameNum < 0) frameNum = 0;
    if (frameNum >= totalFrames) frameNum = totalFrames - 1;
    
    videoCapture.set(cv::CAP_PROP_POS_FRAMES, frameNum);
    videoCapture.read(currentFrameImage);
    
    if (!currentFrameImage.empty()) {
        currentFrame = frameNum;
        updateDisplay();
    }
}

// Find next unannotated frame
int findNextUnannotated(int startFrame, bool forward) {
    int step = forward ? 1 : -1;
    int frame = startFrame + step;
    
    while (frame >= 0 && frame < totalFrames) {
        if (annotations.find(frame) == annotations.end() || 
            !annotations[frame].isComplete()) {
            return frame;
        }
        frame += step;
    }
    
    return startFrame; // No unannotated frame found
}

// Save annotations to JSON file
void saveAnnotations(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }
    
    file << "{\n";
    file << "  \"video_metadata\": {\n";
    file << "    \"filename\": \"" << videoFilename << "\",\n";
    file << "    \"fps\": " << fps << ",\n";
    
    int width = (int)videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
    file << "    \"resolution\": [" << width << ", " << height << "],\n";
    file << "    \"total_frames\": " << totalFrames << "\n";
    file << "  },\n";
    file << "  \"annotations\": [\n";
    
    bool first = true;
    for (const auto& pair : annotations) {
        if (!pair.second.isComplete()) {
            continue; // Only save complete annotations
        }
        
        if (!first) {
            file << ",\n";
        }
        first = false;
        
        int frameNum = pair.first;
        const EyeAnnotation& ann = pair.second;
        double timestamp = frameNum / fps;
        
        file << "    {\n";
        file << "      \"frame_number\": " << frameNum << ",\n";
        file << "      \"timestamp\": " << std::fixed << std::setprecision(3) << timestamp << ",\n";
        file << "      \"left_eye\": {\"x\": " << ann.left_eye.x << ", \"y\": " << ann.left_eye.y << "},\n";
        file << "      \"right_eye\": {\"x\": " << ann.right_eye.x << ", \"y\": " << ann.right_eye.y << "}\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
    
    int completeCount = 0;
    for (const auto& pair : annotations) {
        if (pair.second.isComplete()) completeCount++;
    }
    
    std::cout << "\n=== Annotations saved ===\n";
    std::cout << "File: " << filename << "\n";
    std::cout << "Complete annotations: " << completeCount << "\n";
    
    isDirty = false;
}

// Load annotations from JSON file
bool loadAnnotations(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << "\n";
        return false;
    }
    
    annotations.clear();
    
    std::string line;
    int frameNum = -1;
    int leftX = -1, leftY = -1, rightX = -1, rightY = -1;
    
    // Simple JSON parser (not robust, but works for our format)
    while (std::getline(file, line)) {
        // Look for frame_number
        size_t framePos = line.find("\"frame_number\":");
        if (framePos != std::string::npos) {
            size_t colonPos = line.find(':', framePos);
            size_t commaPos = line.find(',', colonPos);
            if (commaPos == std::string::npos) commaPos = line.length();
            frameNum = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
        }
        
        // Look for left_eye x
        size_t leftEyePos = line.find("\"left_eye\":");
        if (leftEyePos != std::string::npos) {
            size_t xPos = line.find("\"x\":", leftEyePos);
            if (xPos != std::string::npos) {
                size_t colonPos = line.find(':', xPos);
                size_t commaPos = line.find(',', colonPos);
                leftX = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
                
                size_t yPos = line.find("\"y\":", xPos);
                colonPos = line.find(':', yPos);
                commaPos = line.find('}', colonPos);
                leftY = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
            }
        }
        
        // Look for right_eye x
        size_t rightEyePos = line.find("\"right_eye\":");
        if (rightEyePos != std::string::npos) {
            size_t xPos = line.find("\"x\":", rightEyePos);
            if (xPos != std::string::npos) {
                size_t colonPos = line.find(':', xPos);
                size_t commaPos = line.find(',', colonPos);
                rightX = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
                
                size_t yPos = line.find("\"y\":", xPos);
                colonPos = line.find(':', yPos);
                commaPos = line.find('}', colonPos);
                rightY = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
                
                // We have a complete annotation
                if (frameNum >= 0 && leftX >= 0 && rightX >= 0) {
                    EyeAnnotation ann;
                    ann.left_eye = cv::Point(leftX, leftY);
                    ann.right_eye = cv::Point(rightX, rightY);
                    ann.left_set = true;
                    ann.right_set = true;
                    annotations[frameNum] = ann;
                    
                    // Reset for next annotation
                    frameNum = -1;
                    leftX = leftY = rightX = rightY = -1;
                }
            }
        }
    }
    
    file.close();
    
    std::cout << "Loaded " << annotations.size() << " annotations from " << filename << "\n";
    return true;
}

// Get default JSON filename based on video filename
std::string getJsonFilename(const std::string& videoPath) {
    size_t lastSlash = videoPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? videoPath : videoPath.substr(lastSlash + 1);
    
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
    }
    
    return "testing/test_data/ground_truth_" + filename + ".json";
}

// Display help information
void displayHelp() {
    std::cout << "\n=== Eye Annotation Tool - Keyboard Controls ===\n\n";
    std::cout << "Navigation:\n";
    std::cout << "  Right Arrow / D  - Next frame\n";
    std::cout << "  Left Arrow / A   - Previous frame\n";
    std::cout << "  Page Down        - Skip +10 frames\n";
    std::cout << "  Page Up          - Skip -10 frames\n";
    std::cout << "  Home             - Jump +30 frames\n";
    std::cout << "  End              - Jump -30 frames\n";
    std::cout << "  N                - Jump to next unannotated frame\n";
    std::cout << "  P                - Jump to previous unannotated frame\n\n";
    std::cout << "Annotation:\n";
    std::cout << "  Left Click       - Mark eye position (L then R)\n";
    std::cout << "  C                - Clear current frame annotation\n";
    std::cout << "  Delete           - Delete current frame annotation\n\n";
    std::cout << "File Operations:\n";
    std::cout << "  S                - Save annotations to JSON\n";
    std::cout << "  L                - Load annotations from JSON\n\n";
    std::cout << "Other:\n";
    std::cout << "  H / ?            - Show this help\n";
    std::cout << "  Q / ESC          - Quit (with save prompt)\n\n";
    std::cout << "Workflow:\n";
    std::cout << "  1. Click on LEFT eye (blue marker appears)\n";
    std::cout << "  2. Click on RIGHT eye (green marker appears)\n";
    std::cout << "  3. Move to next frame and repeat\n";
    std::cout << "  4. Save with 'S' when done\n\n";
}

// Main function
int main(int argc, char** argv) {
    std::cout << "\n=== Eye Tracking Annotation Tool ===\n\n";
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <video_file> [annotation_json]\n\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " test_video.mp4\n";
        std::cout << "  " << argv[0] << " test_video.mp4 existing_annotations.json\n\n";
        return -1;
    }
    
    std::string videoPath = argv[1];
    videoFilename = videoPath;
    
    // Open video file
    videoCapture.open(videoPath);
    if (!videoCapture.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << "\n";
        return -1;
    }
    
    // Get video properties
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    int width = (int)videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
    
    std::cout << "Video loaded successfully:\n";
    std::cout << "  File: " << videoPath << "\n";
    std::cout << "  Resolution: " << width << "x" << height << "\n";
    std::cout << "  FPS: " << fps << "\n";
    std::cout << "  Total frames: " << totalFrames << "\n\n";
    
    // Load existing annotations if provided
    if (argc >= 3) {
        std::string annotationFile = argv[2];
        loadAnnotations(annotationFile);
    }
    
    // Create window
    cv::namedWindow("Eye Annotation Tool", CV_WINDOW_NORMAL);
    cv::setMouseCallback("Eye Annotation Tool", mouseCallback, nullptr);
    
    // Load first frame
    goToFrame(0);
    
    displayHelp();
    
    // Main loop
    bool running = true;
    while (running) {
        int key = cv::waitKey(10);
        
        if (key == -1) {
            continue;
        }
        
        switch (key) {
            // Navigation
            case 'a':
            case 'A':
                goToFrame(currentFrame - 1);
                break;
                
            case 'd':
            case 'D':
                goToFrame(currentFrame + 1);
                break;
                
            case 'n':
            case 'N': {
                int next = findNextUnannotated(currentFrame, true);
                if (next != currentFrame) {
                    goToFrame(next);
                    std::cout << "Jumped to next unannotated frame: " << next << "\n";
                } else {
                    std::cout << "No more unannotated frames ahead\n";
                }
                break;
            }
                
            case 'p':
            case 'P': {
                int prev = findNextUnannotated(currentFrame, false);
                if (prev != currentFrame) {
                    goToFrame(prev);
                    std::cout << "Jumped to previous unannotated frame: " << prev << "\n";
                } else {
                    std::cout << "No more unannotated frames before\n";
                }
                break;
            }
                
            // Annotation operations
            case 'c':
            case 'C':
            case 127: // Delete key
                annotations[currentFrame].clear();
                std::cout << "Cleared annotation for frame " << currentFrame << "\n";
                isDirty = true;
                updateDisplay();
                break;
                
            // File operations
            case 's':
            case 'S': {
                std::string jsonFile = getJsonFilename(videoPath);
                saveAnnotations(jsonFile);
                break;
            }
                
            case 'l':
            case 'L': {
                std::string jsonFile = getJsonFilename(videoPath);
                std::cout << "Enter JSON file to load (or press Enter for default: " << jsonFile << "): ";
                std::string input;
                std::getline(std::cin, input);
                if (!input.empty()) {
                    jsonFile = input;
                }
                if (loadAnnotations(jsonFile)) {
                    updateDisplay();
                }
                break;
            }
                
            // Help
            case 'h':
            case 'H':
            case '?':
                displayHelp();
                break;
                
            // Quit
            case 'q':
            case 'Q':
            case 27: // ESC
                if (isDirty) {
                    std::cout << "\nYou have unsaved changes. Save before quitting? (y/n): ";
                    char response;
                    std::cin >> response;
                    if (response == 'y' || response == 'Y') {
                        std::string jsonFile = getJsonFilename(videoPath);
                        saveAnnotations(jsonFile);
                    }
                }
                running = false;
                break;
        }
    }
    
    videoCapture.release();
    cv::destroyAllWindows();
    
    std::cout << "\nAnnotation tool closed.\n";
    return 0;
}