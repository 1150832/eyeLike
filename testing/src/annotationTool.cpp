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
    bool eyes_closed; // NOVO: Flag para olhos fechados
    
    EyeAnnotation() : left_eye(-1, -1), right_eye(-1, -1), left_set(false), right_set(false), eyes_closed(false) {}
    
    // Completo se os olhos estiverem fechados OU se ambos os pontos estiverem definidos
    bool isComplete() const {
        return eyes_closed || (left_set && right_set);
    }
    
    void clear() {
        left_eye = cv::Point(-1, -1);
        right_eye = cv::Point(-1, -1);
        left_set = false;
        right_set = false;
        eyes_closed = false; // Limpa também a flag
    }
};

// --- Global Variables ---
std::map<int, EyeAnnotation> annotations;
int currentFrame = 0;
cv::Mat currentFrameImage; 
cv::Mat displayImage;      
cv::VideoCapture videoCapture;
std::string videoFilename;
int totalFrames = 0;
double fps = 30.0;
bool isDirty = false;
bool showHelp = true;      

// Image Enhancement Parameters
double gamma_val = 1.0;    // 1.0 = neutral
int brightness_val = 0;    // -100 to 100
bool use_clahe = false;    // Contrast Limited Adaptive Histogram Equalization

// --- Function Declarations ---
void updateDisplay();
void processImage();
void drawUI(cv::Mat& image);
void saveAnnotations(const std::string& filename);
bool loadAnnotations(const std::string& filename);
void goToFrame(int frameNum);
int findNextUnannotated(int startFrame, bool forward);
std::string getJsonFilename(const std::string& videoPath);
void propagateAnnotations(int startFrame, int count); 

// --- Image Processing ---
void processImage() {
    if (currentFrameImage.empty()) return;

    cv::Mat temp = currentFrameImage.clone();

    // 1. CLAHE (Adaptive Contrast) - Best for dark eyes
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

    // 2. Brightness
    if (brightness_val != 0) {
        temp.convertTo(temp, -1, 1, brightness_val);
    }

    // 3. Gamma Correction
    if (gamma_val != 1.0) {
        cv::Mat lookUpTable(1, 256, CV_8U);
        uchar* p = lookUpTable.ptr();
        for( int i = 0; i < 256; ++i)
            p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma_val) * 255.0);
        cv::LUT(temp, lookUpTable, temp);
    }

    displayImage = temp;
}

// --- Predictive Logic ---
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
    std::cout << "Propagated annotations to next " << filled << " frames.\n";
    isDirty = true;
}

// --- Mouse Callback ---
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event != cv::EVENT_LBUTTONDOWN) return;
    
    EyeAnnotation& ann = annotations[currentFrame];
    
    // Bloquear marcações manuais se a flag "Olhos Fechados" estiver ativa
    if (ann.eyes_closed) {
        std::cout << "Olhos marcados como fechados. Remova a flag (tecla 'x') para adicionar pontos manuais.\n";
        return;
    }
    
    // Smart Edit Logic
    if (!ann.left_set) {
        ann.left_eye = cv::Point(x, y);
        ann.left_set = true;
    } else if (!ann.right_set) {
        ann.right_eye = cv::Point(x, y);
        ann.right_set = true;
    } else {
        ann.left_eye = cv::Point(x, y);
        ann.left_set = true;
        ann.right_set = false; 
        ann.right_eye = cv::Point(-1, -1);
    }
    
    isDirty = true;
    updateDisplay();
}

// --- UI Drawing ---
void drawUI(cv::Mat& img) {
    if (!showHelp) return;

    int y_start = 20;
    int line_h = 18;
    cv::Scalar txt_col(0, 255, 100); // Bright Green
    
    // Background box
    cv::Mat overlay;
    img.copyTo(overlay);
    cv::rectangle(overlay, cv::Point(0, 0), cv::Point(300, 520), cv::Scalar(20,20,20), -1);
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
    drawLine("[X] Toggle EYES CLOSED", cv::Scalar(255, 100, 100)); // NOVO
    drawLine("[Backsp] Clear Frame");
    drawLine("-----------------------------");
    drawLine("[F] PREDICT NEXT 15 FRAMES", cv::Scalar(0, 150, 255));
    drawLine("    (Copies current state fwd)");
    drawLine("-----------------------------");
    drawLine("IMAGE ENHANCEMENT:");
    std::stringstream ss;
    ss << "[W/S] Brightness: " << brightness_val;
    drawLine(ss.str());
    ss.str(""); ss << "[E/R] Gamma: " << std::fixed << std::setprecision(1) << gamma_val;
    drawLine(ss.str());
    drawLine("[C] Toggle CLAHE (Contrast): " + std::string(use_clahe ? "ON" : "OFF"));
    drawLine("[T] Reset Image");
    drawLine("-----------------------------");
    drawLine("[M] Save Progress (Manual)");
    drawLine("[ESC] Quit");
    
    // Status
    y_start = img.rows - 50;
    std::string status = "STATUS: ";
    
    // Atualização visual do estado
    if (annotations[currentFrame].eyes_closed) {
        status += "EYES CLOSED (Skipped)";
        cv::putText(img, status, cv::Point(10, y_start), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(100,100,255), 2);
    } else {
        if (annotations[currentFrame].isComplete()) status += "COMPLETE";
        else if (!annotations[currentFrame].left_set) status += "Click LEFT Eye";
        else status += "Click RIGHT Eye";
        cv::putText(img, status, cv::Point(10, y_start), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,255,255), 2);
    }
    
    std::stringstream ss2;
    ss2 << "Frame: " << currentFrame << "/" << totalFrames;
    cv::putText(img, ss2.str(), cv::Point(10, y_start + 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255,255,255), 1);
}

void updateDisplay() {
    if (currentFrameImage.empty()) return;
    
    processImage(); 
    
    // Desenhar Marcadores apenas se os olhos não estiverem marcados como fechados
    if (annotations.count(currentFrame) && !annotations[currentFrame].eyes_closed) {
        const EyeAnnotation& ann = annotations[currentFrame];
        if (ann.left_set) {
            cv::circle(displayImage, ann.left_eye, 4, cv::Scalar(255, 0, 0), -1);
            cv::circle(displayImage, ann.left_eye, 8, cv::Scalar(255, 0, 0), 1);
            cv::line(displayImage, cv::Point(ann.left_eye.x-10, ann.left_eye.y), cv::Point(ann.left_eye.x+10, ann.left_eye.y), cv::Scalar(255,0,0), 1);
            cv::line(displayImage, cv::Point(ann.left_eye.x, ann.left_eye.y-10), cv::Point(ann.left_eye.x, ann.left_eye.y+10), cv::Scalar(255,0,0), 1);
        }
        if (ann.right_set) {
            cv::circle(displayImage, ann.right_eye, 4, cv::Scalar(0, 255, 0), -1);
            cv::circle(displayImage, ann.right_eye, 8, cv::Scalar(0, 255, 0), 1);
            cv::line(displayImage, cv::Point(ann.right_eye.x-10, ann.right_eye.y), cv::Point(ann.right_eye.x+10, ann.right_eye.y), cv::Scalar(0,255,0), 1);
            cv::line(displayImage, cv::Point(ann.right_eye.x, ann.right_eye.y-10), cv::Point(ann.right_eye.x, ann.right_eye.y+10), cv::Scalar(0,255,0), 1);
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
    
    if (!currentFrameImage.empty()) {
        currentFrame = frameNum;
        updateDisplay();
    }
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
    if (!file.is_open()) {
        std::cerr << "Error writing file: " << filename << "\n";
        return;
    }
    
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
        file << "      \"eyes_closed\": " << (ann.eyes_closed ? "true" : "false") << ",\n"; // NOVO
        file << "      \"left_eye\": {\"x\": " << ann.left_eye.x << ", \"y\": " << ann.left_eye.y << "},\n";
        file << "      \"right_eye\": {\"x\": " << ann.right_eye.x << ", \"y\": " << ann.right_eye.y << "}\n";
        file << "    }";
    }
    file << "\n  ]\n}\n";
    file.close();
    std::cout << "Saved " << filename << "\n";
    isDirty = false;
}

bool loadAnnotations(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::cout << "Loading existing annotations from " << filename << "...\n";
    std::string line;
    
    int frameNum = -1;
    int lx = -1, ly = -1, rx = -1, ry = -1;
    bool closed = false; // Estado local de parsing
    
    while (std::getline(file, line)) {
        if (line.find("\"frame_number\":") != std::string::npos) {
             sscanf(line.c_str(), "      \"frame_number\": %d,", &frameNum);
             closed = false; // Reset no início de nova frame
        }
        else if (line.find("\"eyes_closed\":") != std::string::npos) { // NOVO PARSER
             if (line.find("true") != std::string::npos) closed = true;
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
                 ann.eyes_closed = closed; // Carrega a flag
                 
                 annotations[frameNum] = ann;
                 frameNum = -1;
             }
        }
    }
    file.close();
    return true;
}

std::string getJsonFilename(const std::string& videoPath) {
    // Find dir where video is located
    size_t lastSlash = videoPath.find_last_of("/\\");
    std::string dir = (lastSlash == std::string::npos) ? "." : videoPath.substr(0, lastSlash);
    
    // Extract filename without extension
    std::string filename = (lastSlash == std::string::npos) ? videoPath : videoPath.substr(lastSlash + 1);
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    
    // Save JSON in the same dir
    return dir + "/ground_truth_" + filename + ".json";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <video_file> [annotation_json]\n";
        return -1;
    }
    
    videoFilename = argv[1];
    videoCapture.open(videoFilename);
    if (!videoCapture.isOpened()) {
        std::cerr << "Error opening video: " << videoFilename << "\n";
        return -1;
    }
    
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    std::string jsonPath = getJsonFilename(videoFilename);
    if (argc >= 3) jsonPath = argv[2]; 
    
    loadAnnotations(jsonPath);
    
    cv::namedWindow("Eye Annotation Tool", CV_WINDOW_NORMAL);
    cv::setMouseCallback("Eye Annotation Tool", mouseCallback, nullptr);
    
    goToFrame(0);
    
    bool running = true;
    while (running) {
        int key = cv::waitKey(20);
        if (key == -1) continue;
        
        switch (key) {
            case 27: running = false; break; // ESC to Quit
            
            // Navigation
            case 'd': goToFrame(currentFrame + 1); break;
            case 'a': goToFrame(currentFrame - 1); break;
            case 32:  goToFrame(findNextUnannotated(currentFrame, true)); break; // Spacebar
            
            // UI Toggle
            case 'h': showHelp = !showHelp; updateDisplay(); break;
            
            // Prediction Feature
            case 'f': propagateAnnotations(currentFrame, 15); break; 
            
            // Toggle Eyes Closed
            case 'x':
                annotations[currentFrame].eyes_closed = !annotations[currentFrame].eyes_closed;
                if (annotations[currentFrame].eyes_closed) {
                    // Limpa cliques acidentais se existirem
                    annotations[currentFrame].left_set = false;
                    annotations[currentFrame].right_set = false;
                }
                isDirty = true;
                updateDisplay();
                break;

            // Editing
            case '1': 
                annotations[currentFrame].left_set = false;
                updateDisplay();
                break;
            case '2': 
                annotations[currentFrame].right_set = false;
                updateDisplay();
                break;
            case 8: // Backspace (Mac sometimes sends 127)
            case 127: 
                annotations[currentFrame].clear(); 
                updateDisplay(); 
                break;
                
            // Image Enhancement (No Conflicts)
            case 'w': brightness_val += 5; updateDisplay(); break;
            case 's': brightness_val -= 5; updateDisplay(); break;
            case 'e': gamma_val += 0.1; updateDisplay(); break;
            case 'r': if(gamma_val > 0.2) gamma_val -= 0.1; updateDisplay(); break;
            case 'c': use_clahe = !use_clahe; updateDisplay(); break;
            case 't': brightness_val=0; gamma_val=1.0; use_clahe=false; updateDisplay(); break;
            
            // File
            case 'm': saveAnnotations(jsonPath); break;
        }
    }
    
    if (isDirty) {
        saveAnnotations(jsonPath);
    }
    
    videoCapture.release();
    cv::destroyAllWindows();
    return 0;
}