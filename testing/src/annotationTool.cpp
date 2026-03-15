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
    bool left_missing;  
    bool right_missing; 
    bool eyes_closed;
    
    EyeAnnotation() : left_eye(-1, -1), right_eye(-1, -1), left_set(false), right_set(false), 
                      left_missing(false), right_missing(false), eyes_closed(false) {}
    
    bool isComplete() const {
        if (eyes_closed) return true;
        bool l_ok = left_set || left_missing;
        bool r_ok = right_set || right_missing;
        return l_ok && r_ok;
    }
    
    void clear() {
        left_eye = cv::Point(-1, -1);
        right_eye = cv::Point(-1, -1);
        left_set = false;
        right_set = false;
        left_missing = false;
        right_missing = false;
        eyes_closed = false;
    }
};

// --- Global Variables ---
std::map<int, EyeAnnotation> annotations;
int currentFrame = 0;
cv::Mat currentFrameImage; 
cv::Mat displayImage;      
cv::VideoCapture videoCapture;
std::string videoFilename;
std::string currentJsonPath;
int totalFrames = 0;
double fps = 30.0;
bool isDirty = false;
bool showHelp = true;      

// Auto-Save properties
int unsavedChanges = 0;
const int AUTOSAVE_THRESHOLD = 20;

// Image Enhancement Parameters
double gamma_val = 1.0;
int brightness_val = 0;
bool use_clahe = false;

// --- Function Declarations ---
void updateDisplay();
void processImage();
void drawUI(cv::Mat& image);
void drawTimeline(cv::Mat& image);
void saveAnnotations(const std::string& filename);
bool loadAnnotations(const std::string& filename);
void goToFrame(int frameNum);
int findNextUnannotated(int startFrame, bool forward);
std::string getJsonFilename(const std::string& videoPath);
void propagateAnnotations(int startFrame, int count); 
void triggerAutoSaveCheck();

// --- Image Processing ---
void processImage() {
    if (currentFrameImage.empty()) return;
    cv::Mat temp = currentFrameImage.clone();

    if (use_clahe) {
        cv::Mat lab_image;
        cv::cvtColor(temp, lab_image, cv::COLOR_BGR2Lab);
        std::vector<cv::Mat> lab_planes(3);
        cv::split(lab_image, lab_planes);
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
        clahe->setClipLimit(4.0);
        clahe->apply(lab_planes[0], lab_planes[0]);
        cv::merge(lab_planes, lab_image);
        cv::cvtColor(lab_image, temp, cv::COLOR_Lab2BGR);
    }
    if (brightness_val != 0) temp.convertTo(temp, -1, 1, brightness_val);
    if (gamma_val != 1.0) {
        cv::Mat lookUpTable(1, 256, CV_8U);
        uchar* p = lookUpTable.ptr();
        for( int i = 0; i < 256; ++i) p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma_val) * 255.0);
        cv::LUT(temp, lookUpTable, temp);
    }
    displayImage = temp;
}

// --- Predictive & AutoSave Logic ---
void triggerAutoSaveCheck() {
    if (unsavedChanges >= AUTOSAVE_THRESHOLD) {
        std::cout << "\n[AUTO-SAVE] Saving progress after " << unsavedChanges << " changes...\n";
        saveAnnotations(currentJsonPath);
    }
}

void propagateAnnotations(int startFrame, int count) {
    if (annotations.find(startFrame) == annotations.end() || !annotations[startFrame].isComplete()) {
        std::cout << "Cannot propagate: Current frame is incomplete.\n";
        return;
    }
    EyeAnnotation src = annotations[startFrame];
    int maxF = std::min(startFrame + count, totalFrames);
    int filled = 0;
    for (int f = startFrame + 1; f < maxF; f++) {
        annotations[f] = src;
        filled++;
    }
    isDirty = true;
    unsavedChanges += filled;
    triggerAutoSaveCheck();
}

// --- Mouse Callback ---
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event != cv::EVENT_LBUTTONDOWN) return;
    EyeAnnotation& ann = annotations[currentFrame];
    
    if (ann.eyes_closed) {
        std::cout << "Eyes marked as closed. Remove the flag first.\n";
        return;
    }
    
    if (!ann.left_set && !ann.left_missing) {
        ann.left_eye = cv::Point(x, y);
        ann.left_set = true;
    } else if (!ann.right_set && !ann.right_missing) {
        ann.right_eye = cv::Point(x, y);
        ann.right_set = true;
    } else {
        if (!ann.left_missing) { ann.left_eye = cv::Point(x, y); ann.left_set = true; }
        if (!ann.right_missing) { ann.right_set = false; ann.right_eye = cv::Point(-1, -1); }
    }
    
    isDirty = true;
    unsavedChanges++;
    updateDisplay();
    triggerAutoSaveCheck();
}

// --- UI & Timeline Drawing ---
void drawTimeline(cv::Mat& img) {
    int timeline_h = 20; 
    int timeline_y = img.rows - timeline_h; 
    cv::rectangle(img, cv::Point(0, timeline_y), cv::Point(img.cols, img.rows), cv::Scalar(50, 50, 50), -1);

    if (totalFrames > 0) {
        float rect_w = (float)img.cols / totalFrames; 
        for (const auto& pair : annotations) {
            int frameIdx = pair.first;
            const EyeAnnotation& ann = pair.second;
            
            if (ann.isComplete()) {
                cv::Scalar color;
                if (ann.eyes_closed) color = cv::Scalar(0, 255, 255); // Amarelo (Fechados)
                else if (ann.left_missing || ann.right_missing) color = cv::Scalar(255, 165, 0); // Laranja (Oculto Parcial)
                else color = cv::Scalar(0, 255, 0); // Verde (Tudo Normal)
                
                int x1 = (int)(frameIdx * rect_w);
                int x2 = (int)((frameIdx + 1) * rect_w);
                if (x2 == x1) x2 = x1 + 1; 
                cv::rectangle(img, cv::Point(x1, timeline_y), cv::Point(x2, img.rows), color, -1);
            }
        }
        int cur_x = (int)(currentFrame * rect_w);
        cv::line(img, cv::Point(cur_x, timeline_y - 10), cv::Point(cur_x, img.rows), cv::Scalar(0, 0, 255), 2);
    }
}

void drawUI(cv::Mat& img) {
    if (!showHelp) { drawTimeline(img); return; }

    int y_start = 20;
    int line_h = 18;
    
    cv::Mat overlay;
    img.copyTo(overlay);
    cv::rectangle(overlay, cv::Point(0, 0), cv::Point(330, 540), cv::Scalar(20,20,20), -1);
    cv::addWeighted(overlay, 0.7, img, 0.3, 0, img);

    auto drawLine = [&](std::string text, cv::Scalar color = cv::Scalar(0, 255, 100)) {
        cv::putText(img, text, cv::Point(10, y_start), cv::FONT_HERSHEY_PLAIN, 1.1, color, 1);
        y_start += line_h;
    };

    drawLine("=== CONTROLS (Toggle 'H') ===", cv::Scalar(0, 255, 255));
    y_start += 5;
    drawLine("[A] Prev Frame   [D] Next Frame");
    drawLine("[Space] Next Empty Frame");
    drawLine("-----------------------------");
    drawLine("[1] Edit LEFT Eye (Blue)");
    drawLine("[2] Edit RIGHT Eye (Green)");
    drawLine("[3] Toggle LEFT Eye MISSING", cv::Scalar(150, 150, 255)); // NOVO
    drawLine("[4] Toggle RIGHT Eye MISSING", cv::Scalar(150, 255, 150)); // NOVO
    drawLine("[X] Toggle BOTH EYES CLOSED", cv::Scalar(255, 100, 100)); 
    drawLine("[Backsp] Clear Frame");
    drawLine("-----------------------------");
    drawLine("[F] PREDICT NEXT 15 FRAMES", cv::Scalar(0, 150, 255));
    drawLine("-----------------------------");
    drawLine("IMAGE ENHANCEMENT:");
    std::stringstream ss;
    ss << "[W/S] Brightness: " << brightness_val; drawLine(ss.str());
    ss.str(""); ss << "[E/R] Gamma: " << std::fixed << std::setprecision(1) << gamma_val; drawLine(ss.str());
    drawLine("[C] Toggle CLAHE (Contrast): " + std::string(use_clahe ? "ON" : "OFF"));
    drawLine("[T] Reset Image");
    drawLine("-----------------------------");
    drawLine("[M] Save Progress (Manual)");
    drawLine("[ESC/Q] Quit");
    
    // Status Indicator
    y_start = img.rows - 70; 
    EyeAnnotation& ann = annotations[currentFrame];
    
    if (ann.eyes_closed) {
        cv::putText(img, "STATUS: EYES CLOSED", cv::Point(10, y_start), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(100,100,255), 2);
    } else {
        std::string statusStr = "STATUS: ";
        cv::Scalar color = cv::Scalar(0,255,255);
        if (ann.isComplete()) {
            statusStr += "COMPLETE ";
            if (ann.left_missing) statusStr += "(L: Missing) ";
            if (ann.right_missing) statusStr += "(R: Missing) ";
        }
        else if (!ann.left_set && !ann.left_missing) statusStr += "Click LEFT Eye";
        else if (!ann.right_set && !ann.right_missing) statusStr += "Click RIGHT Eye";
        
        cv::putText(img, statusStr, cv::Point(10, y_start), cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);
    }
    
    std::stringstream ss2;
    ss2 << "Frame: " << currentFrame << "/" << totalFrames;
    cv::putText(img, ss2.str(), cv::Point(10, y_start + 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,255,255), 1);
    
    drawTimeline(img);
}

void updateDisplay() {
    if (currentFrameImage.empty()) return;
    processImage(); 
    
    if (annotations.count(currentFrame) && !annotations[currentFrame].eyes_closed) {
        const EyeAnnotation& ann = annotations[currentFrame];
        if (ann.left_set && !ann.left_missing) {
            cv::circle(displayImage, ann.left_eye, 4, cv::Scalar(255, 0, 0), -1);
            cv::circle(displayImage, ann.left_eye, 8, cv::Scalar(255, 0, 0), 1);
        }
        if (ann.right_set && !ann.right_missing) {
            cv::circle(displayImage, ann.right_eye, 4, cv::Scalar(0, 255, 0), -1);
            cv::circle(displayImage, ann.right_eye, 8, cv::Scalar(0, 255, 0), 1);
        }
    }
    
    drawUI(displayImage);
    cv::imshow("Eye Annotation Tool", displayImage);
}

void goToFrame(int frameNum) {
    if (frameNum < 0) frameNum = 0;
    if (frameNum >= totalFrames) frameNum = totalFrames - 1;
    videoCapture.set(cv::CAP_PROP_POS_FRAMES, frameNum);
    videoCapture.read(currentFrameImage);
    if (!currentFrameImage.empty()) { currentFrame = frameNum; updateDisplay(); }
}

int findNextUnannotated(int startFrame, bool forward) {
    int step = forward ? 1 : -1;
    int frame = startFrame + step;
    while (frame >= 0 && frame < totalFrames) {
        if (!annotations[frame].isComplete()) return frame;
        frame += step;
    }
    return startFrame;
}

void saveAnnotations(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n  \"video_metadata\": {\n";
    file << "    \"filename\": \"" << videoFilename << "\",\n";
    file << "    \"total_frames\": " << totalFrames << "\n  },\n";
    file << "  \"annotations\": [\n";
    
    bool first = true;
    for (const auto& pair : annotations) {
        if (!pair.second.isComplete()) continue;
        if (!first) file << ",\n";
        first = false;
        
        const EyeAnnotation& ann = pair.second;
        file << "    {\n";
        file << "      \"frame_number\": " << pair.first << ",\n";
        file << "      \"timestamp\": " << std::fixed << std::setprecision(3) << (pair.first / fps) << ",\n";
        file << "      \"eyes_closed\": " << (ann.eyes_closed ? "true" : "false") << ",\n";
        file << "      \"left_missing\": " << (ann.left_missing ? "true" : "false") << ",\n";  // NOVO
        file << "      \"right_missing\": " << (ann.right_missing ? "true" : "false") << ",\n"; // NOVO
        file << "      \"left_eye\": {\"x\": " << ann.left_eye.x << ", \"y\": " << ann.left_eye.y << "},\n";
        file << "      \"right_eye\": {\"x\": " << ann.right_eye.x << ", \"y\": " << ann.right_eye.y << "}\n";
        file << "    }";
    }
    file << "\n  ]\n}\n";
    file.close();
    
    std::cout << "Saved " << filename << "\n";
    isDirty = false;
    unsavedChanges = 0;
}

bool loadAnnotations(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::string line;
    int frameNum = -1, lx = -1, ly = -1, rx = -1, ry = -1;
    bool closed = false, l_miss = false, r_miss = false; 
    
    while (std::getline(file, line)) {
        if (line.find("\"frame_number\":") != std::string::npos) {
             sscanf(line.c_str(), "      \"frame_number\": %d,", &frameNum);
             closed = false; l_miss = false; r_miss = false; 
        }
        else if (line.find("\"eyes_closed\":") != std::string::npos) {
             if (line.find("true") != std::string::npos) closed = true;
        }
        else if (line.find("\"left_missing\":") != std::string::npos) { // NOVO PARSER
             if (line.find("true") != std::string::npos) l_miss = true;
        }
        else if (line.find("\"right_missing\":") != std::string::npos) { // NOVO PARSER
             if (line.find("true") != std::string::npos) r_miss = true;
        }
        else if (line.find("\"left_eye\":") != std::string::npos) {
             sscanf(line.c_str(), "      \"left_eye\": {\"x\": %d, \"y\": %d},", &lx, &ly);
        }
        else if (line.find("\"right_eye\":") != std::string::npos) {
             sscanf(line.c_str(), "      \"right_eye\": {\"x\": %d, \"y\": %d}", &rx, &ry);
             if (frameNum >= 0) {
                 EyeAnnotation ann;
                 ann.left_eye = cv::Point(lx, ly); 
                 ann.right_eye = cv::Point(rx, ry); 
                 if (lx != -1 && ly != -1) ann.left_set = true;
                 if (rx != -1 && ry != -1) ann.right_set = true;
                 ann.eyes_closed = closed; 
                 ann.left_missing = l_miss;
                 ann.right_missing = r_miss;
                 
                 annotations[frameNum] = ann;
                 frameNum = -1;
             }
        }
    }
    file.close();
    return true;
}

std::string getJsonFilename(const std::string& videoPath) {
    size_t lastSlash = videoPath.find_last_of("/\\");
    std::string dir = (lastSlash == std::string::npos) ? "." : videoPath.substr(0, lastSlash);
    std::string filename = (lastSlash == std::string::npos) ? videoPath : videoPath.substr(lastSlash + 1);
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    return dir + "/ground_truth_" + filename + ".json";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "\n=== Eye Annotation Tool ===\n";
        std::cout << "Error: Missing path to video.\n";
        std::cout << "Use: " << argv[0] << " <video_file> [annotation_json]\n";
        std::cout << "Example: " << argv[0] << " testing/test_data/test1.mov\n\n";
        return -1;
    }
    
    videoFilename = argv[1];
    videoCapture.open(videoFilename);
    if (!videoCapture.isOpened()) {
        std::cerr << "Error: Not possible to open video: " << videoFilename << "\n";
        return -1;
    }
    
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    currentJsonPath = getJsonFilename(videoFilename);
    if (argc >= 3) currentJsonPath = argv[2]; 
    
    loadAnnotations(currentJsonPath);
    cv::namedWindow("Eye Annotation Tool", CV_WINDOW_NORMAL);
    cv::setMouseCallback("Eye Annotation Tool", mouseCallback, nullptr);
    goToFrame(0);
    
    bool running = true;
    while (running) {
        int key = cv::waitKey(20);
        if (key == -1) continue;
        
        switch (key) {
            case 'q': case 'Q': case 27: running = false; break;
            case 'd': goToFrame(currentFrame + 1); break;
            case 'a': goToFrame(currentFrame - 1); break;
            case 32:  goToFrame(findNextUnannotated(currentFrame, true)); break;
            case 'h': showHelp = !showHelp; updateDisplay(); break;
            case 'f': propagateAnnotations(currentFrame, 15); break; 
            
            case 'x':
                annotations[currentFrame].eyes_closed = !annotations[currentFrame].eyes_closed;
                if (annotations[currentFrame].eyes_closed) {
                    annotations[currentFrame].left_set = false; annotations[currentFrame].right_set = false;
                    annotations[currentFrame].left_missing = false; annotations[currentFrame].right_missing = false;
                }
                isDirty = true; unsavedChanges++; updateDisplay(); triggerAutoSaveCheck(); break;

            case '3': // NOVO: Left Missing Toggle
                annotations[currentFrame].left_missing = !annotations[currentFrame].left_missing;
                if (annotations[currentFrame].left_missing) annotations[currentFrame].left_set = false;
                isDirty = true; unsavedChanges++; updateDisplay(); triggerAutoSaveCheck(); break;
                
            case '4': // NOVO: Right Missing Toggle
                annotations[currentFrame].right_missing = !annotations[currentFrame].right_missing;
                if (annotations[currentFrame].right_missing) annotations[currentFrame].right_set = false;
                isDirty = true; unsavedChanges++; updateDisplay(); triggerAutoSaveCheck(); break;

            case '1': annotations[currentFrame].left_set = false; annotations[currentFrame].left_missing = false; updateDisplay(); break;
            case '2': annotations[currentFrame].right_set = false; annotations[currentFrame].right_missing = false; updateDisplay(); break;
            case 8: case 127: annotations[currentFrame].clear(); unsavedChanges++; updateDisplay(); triggerAutoSaveCheck(); break;
                
            case 'w': brightness_val += 5; updateDisplay(); break;
            case 's': brightness_val -= 5; updateDisplay(); break;
            case 'e': gamma_val += 0.1; updateDisplay(); break;
            case 'r': if(gamma_val > 0.2) gamma_val -= 0.1; updateDisplay(); break;
            case 'c': use_clahe = !use_clahe; updateDisplay(); break;
            case 't': brightness_val=0; gamma_val=1.0; use_clahe=false; updateDisplay(); break;
            case 'm': saveAnnotations(currentJsonPath); break;
        }
    }
    
    if (isDirty) saveAnnotations(currentJsonPath);
    videoCapture.release();
    cv::destroyAllWindows();
    return 0;
}