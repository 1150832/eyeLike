#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <iomanip>
#include <sstream>

/* Support for OpenCV 4.x */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#endif

// Structure for ground truth annotation
struct GroundTruth {
    cv::Point left_eye;
    cv::Point right_eye;
    bool eyes_closed; // NOVO: Flag para olhos fechados
    
    GroundTruth() : left_eye(-1, -1), right_eye(-1, -1), eyes_closed(false) {}
    GroundTruth(cv::Point l, cv::Point r, bool closed = false) : left_eye(l), right_eye(r), eyes_closed(closed) {}
};

// Structure for prediction from eyeLike
struct Prediction {
    cv::Point left_eye;
    cv::Point right_eye;
    
    Prediction() : left_eye(-1, -1), right_eye(-1, -1) {}
    Prediction(cv::Point l, cv::Point r) : left_eye(l), right_eye(r) {}
};

// Structure for test metrics
struct TestMetrics {
    int total_annotated_frames;
    int frames_with_predictions;
    int closed_eyes_frames; // NOVO: Contador
    
    std::vector<double> left_eye_errors;
    std::vector<double> right_eye_errors;
    std::vector<double> avg_errors;
    
    double avg_left_error;
    double avg_right_error;
    double avg_total_error;
    
    double max_left_error;
    double max_right_error;
    
    double std_left_error;
    double std_right_error;
    
    int within_5px;
    int within_10px;
    int within_20px;
    int within_50px;
    
    TestMetrics() : total_annotated_frames(0), frames_with_predictions(0), closed_eyes_frames(0),
                    avg_left_error(0), avg_right_error(0), avg_total_error(0),
                    max_left_error(0), max_right_error(0),
                    std_left_error(0), std_right_error(0),
                    within_5px(0), within_10px(0), within_20px(0), within_50px(0) {}
};

// Global variables
std::map<int, GroundTruth> groundTruths;
std::map<int, Prediction> predictions;
cv::VideoCapture videoCapture;
int currentFrame = 0;
int totalFrames = 0;
double fps = 30.0;
cv::Mat currentFrameImage;
cv::Mat displayImage;
TestMetrics metrics;

// Function declarations
bool loadGroundTruth(const std::string& filename);
bool loadPredictions(const std::string& filename);
void calculateMetrics();
void displayMetrics();
void saveMetricsReport(const std::string& filename);
void drawComparison(cv::Mat& image, int frameNum);
void goToFrame(int frameNum);
void updateDisplay();
double calculateDistance(const cv::Point& p1, const cv::Point& p2);

// Calculate Euclidean distance
double calculateDistance(const cv::Point& p1, const cv::Point& p2) {
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

// Load ground truth from JSON
bool loadGroundTruth(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ground truth file: " << filename << "\n";
        return false;
    }
    
    groundTruths.clear();
    
    std::string line;
    int frameNum = -1;
    int leftX = -1, leftY = -1, rightX = -1, rightY = -1;
    bool closed = false; // Estado local para os olhos fechados
    
    while (std::getline(file, line)) {
        size_t framePos = line.find("\"frame_number\":");
        if (framePos != std::string::npos) {
            size_t colonPos = line.find(':', framePos);
            size_t commaPos = line.find(',', colonPos);
            if (commaPos == std::string::npos) commaPos = line.length();
            frameNum = std::stoi(line.substr(colonPos + 1, commaPos - colonPos - 1));
            closed = false; // Reset ao ler nova frame
        }
        
        // NOVO: Ler flag de olhos fechados
        if (line.find("\"eyes_closed\":") != std::string::npos) {
            if (line.find("true") != std::string::npos) closed = true;
        }
        
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
                
                // NOVO: Aceitar a frame se os olhos estiverem fechados OU se as coordenadas forem válidas
                if (frameNum >= 0 && (closed || (leftX >= 0 && rightX >= 0))) {
                    groundTruths[frameNum] = GroundTruth(cv::Point(leftX, leftY), 
                                                         cv::Point(rightX, rightY), closed);
                    frameNum = -1;
                    leftX = leftY = rightX = rightY = -1;
                    closed = false;
                }
            }
        }
    }
    
    file.close();
    std::cout << "Loaded " << groundTruths.size() << " ground truth annotations\n";
    return true;
}

// Load predictions from CSV
bool loadPredictions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open predictions file: " << filename << "\n";
        return false;
    }
    
    predictions.clear();
    
    std::string line;
    std::getline(file, line); // Skip header
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 9) {
            int frameNum = std::stoi(tokens[0]);
            int rightX = std::stoi(tokens[5]);
            int rightY = std::stoi(tokens[6]);
            int leftX = std::stoi(tokens[7]);
            int leftY = std::stoi(tokens[8]);
            
            predictions[frameNum] = Prediction(cv::Point(leftX, leftY), 
                                              cv::Point(rightX, rightY));
        }
    }
    
    file.close();
    std::cout << "Loaded " << predictions.size() << " predictions\n";
    return true;
}

// Calculate all metrics
void calculateMetrics() {
    metrics = TestMetrics();
    metrics.total_annotated_frames = groundTruths.size();
    
    for (const auto& gtPair : groundTruths) {
        int frameNum = gtPair.first;
        const GroundTruth& gt = gtPair.second;
        
        if (predictions.find(frameNum) == predictions.end()) {
            continue;
        }
        
        // NOVO: Se os olhos estiverem fechados na anotação, não calculamos o erro em pixeis
        if (gt.eyes_closed) {
            metrics.closed_eyes_frames++;
            continue; 
        }
        
        const Prediction& pred = predictions[frameNum];
        metrics.frames_with_predictions++;
        
        // Calculate errors
        double leftError = calculateDistance(gt.left_eye, pred.left_eye);
        double rightError = calculateDistance(gt.right_eye, pred.right_eye);
        double avgError = (leftError + rightError) / 2.0;
        
        metrics.left_eye_errors.push_back(leftError);
        metrics.right_eye_errors.push_back(rightError);
        metrics.avg_errors.push_back(avgError);
        
        // Update max errors
        if (leftError > metrics.max_left_error) metrics.max_left_error = leftError;
        if (rightError > metrics.max_right_error) metrics.max_right_error = rightError;
        
        // Count within thresholds
        if (avgError <= 5.0) metrics.within_5px++;
        if (avgError <= 10.0) metrics.within_10px++;
        if (avgError <= 20.0) metrics.within_20px++;
        if (avgError <= 50.0) metrics.within_50px++;
    }
    
    // Calculate averages (Apenas para olhos abertos)
    if (!metrics.left_eye_errors.empty()) {
        double sumLeft = 0, sumRight = 0, sumAvg = 0;
        for (size_t i = 0; i < metrics.left_eye_errors.size(); i++) {
            sumLeft += metrics.left_eye_errors[i];
            sumRight += metrics.right_eye_errors[i];
            sumAvg += metrics.avg_errors[i];
        }
        
        int n = metrics.left_eye_errors.size();
        metrics.avg_left_error = sumLeft / n;
        metrics.avg_right_error = sumRight / n;
        metrics.avg_total_error = sumAvg / n;
        
        // Calculate standard deviations
        double varLeft = 0, varRight = 0;
        for (size_t i = 0; i < metrics.left_eye_errors.size(); i++) {
            varLeft += std::pow(metrics.left_eye_errors[i] - metrics.avg_left_error, 2);
            varRight += std::pow(metrics.right_eye_errors[i] - metrics.avg_right_error, 2);
        }
        metrics.std_left_error = std::sqrt(varLeft / n);
        metrics.std_right_error = std::sqrt(varRight / n);
    }
}

// Display metrics in console
void displayMetrics() {
    std::cout << "\n===============================================\n";
    std::cout << "           EYE TRACKING TEST RESULTS          \n";
    std::cout << "===============================================\n\n";
    
    std::cout << "Dataset Overview:\n";
    std::cout << "  Total annotated frames:    " << metrics.total_annotated_frames << "\n";
    std::cout << "  Frames with closed eyes:   " << metrics.closed_eyes_frames << " (Excluded from pixel error)\n";
    std::cout << "  Frames analyzed (Open):    " << metrics.frames_with_predictions << "\n";
    
    if (metrics.frames_with_predictions > 0) {
        std::cout << "  Detection rate (Open eyes): " 
                  << std::fixed << std::setprecision(1)
                  << (100.0 * metrics.frames_with_predictions / (metrics.total_annotated_frames - metrics.closed_eyes_frames)) 
                  << "%\n\n";
    }

    std::cout << "Error Statistics (pixels - Open Eyes Only):\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Left Eye  - Avg: " << metrics.avg_left_error 
              << " ± " << metrics.std_left_error
              << ", Max: " << metrics.max_left_error << "\n";
    std::cout << "  Right Eye - Avg: " << metrics.avg_right_error 
              << " ± " << metrics.std_right_error
              << ", Max: " << metrics.max_right_error << "\n";
    std::cout << "  Overall   - Avg: " << metrics.avg_total_error << " px\n\n";
    
    std::cout << "Accuracy Thresholds (Open Eyes Only):\n";
    int n = metrics.frames_with_predictions;
    if (n > 0) {
        std::cout << "  Within  5px: " << std::setw(4) << metrics.within_5px 
                  << " / " << n << " (" << std::setw(5) << (100.0 * metrics.within_5px / n) << "%)\n";
        std::cout << "  Within 10px: " << std::setw(4) << metrics.within_10px 
                  << " / " << n << " (" << std::setw(5) << (100.0 * metrics.within_10px / n) << "%)\n";
        std::cout << "  Within 20px: " << std::setw(4) << metrics.within_20px 
                  << " / " << n << " (" << std::setw(5) << (100.0 * metrics.within_20px / n) << "%)\n";
        std::cout << "  Within 50px: " << std::setw(4) << metrics.within_50px 
                  << " / " << n << " (" << std::setw(5) << (100.0 * metrics.within_50px / n) << "%)\n";
    }
    
    std::cout << "\n===============================================\n\n";
}

// Save metrics report to file
void saveMetricsReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not write report to " << filename << "\n";
        return;
    }
    
    file << "EYE TRACKING TEST REPORT\n";
    file << "========================\n\n";
    
    file << "Dataset Overview:\n";
    file << "  Total annotated frames:    " << metrics.total_annotated_frames << "\n";
    file << "  Frames with closed eyes:   " << metrics.closed_eyes_frames << " (Excluded from pixel error)\n";
    file << "  Frames analyzed (Open):    " << metrics.frames_with_predictions << "\n\n";
    
    file << "Error Statistics (pixels - Open Eyes Only):\n";
    file << std::fixed << std::setprecision(2);
    file << "  Left Eye  - Avg: " << metrics.avg_left_error 
         << " +/- " << metrics.std_left_error
         << ", Max: " << metrics.max_left_error << "\n";
    file << "  Right Eye - Avg: " << metrics.avg_right_error 
         << " +/- " << metrics.std_right_error
         << ", Max: " << metrics.max_right_error << "\n";
    file << "  Overall   - Avg: " << metrics.avg_total_error << " px\n\n";
    
    file << "Accuracy Thresholds (Open Eyes Only):\n";
    int n = metrics.frames_with_predictions;
    if (n > 0) {
        file << "  Within  5px: " << metrics.within_5px << " / " << n 
             << " (" << (100.0 * metrics.within_5px / n) << "%)\n";
        file << "  Within 10px: " << metrics.within_10px << " / " << n 
             << " (" << (100.0 * metrics.within_10px / n) << "%)\n";
        file << "  Within 20px: " << metrics.within_20px << " / " << n 
             << " (" << (100.0 * metrics.within_20px / n) << "%)\n";
        file << "  Within 50px: " << metrics.within_50px << " / " << n 
             << " (" << (100.0 * metrics.within_50px / n) << "%)\n";
    }
    
    file << "\n\nPer-Frame Detailed Results:\n";
    file << "Frame,Left_Error_px,Right_Error_px,Avg_Error_px,State\n";
    
    for (const auto& gtPair : groundTruths) {
        int frameNum = gtPair.first;
        const GroundTruth& gt = gtPair.second;
        
        if (gt.eyes_closed) {
             file << frameNum << ",0,0,0,CLOSED\n";
        } else if (predictions.find(frameNum) != predictions.end()) {
            const Prediction& pred = predictions[frameNum];
            double leftError = calculateDistance(gt.left_eye, pred.left_eye);
            double rightError = calculateDistance(gt.right_eye, pred.right_eye);
            double avgError = (leftError + rightError) / 2.0;
            
            file << frameNum << "," << leftError << "," << rightError << "," << avgError << ",OPEN\n";
        }
    }
    
    file.close();
    std::cout << "Detailed report saved to: " << filename << "\n";
}

// Draw comparison visualization
void drawComparison(cv::Mat& image, int frameNum) {
    if (groundTruths.find(frameNum) == groundTruths.end()) {
        return;
    }
    
    const GroundTruth& gt = groundTruths[frameNum];
    
    // NOVO: Feedback visual para olhos fechados
    if (gt.eyes_closed) {
        cv::putText(image, "GROUND TRUTH: EYES CLOSED", cv::Point(image.cols / 2 - 200, 100),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 3);
                    
        // Desenha as predições (para debug) se elas existirem
        if (predictions.find(frameNum) != predictions.end()) {
            const Prediction& pred = predictions[frameNum];
            cv::circle(image, pred.left_eye, 12, cv::Scalar(0, 0, 255), 3);
            cv::circle(image, pred.right_eye, 12, cv::Scalar(0, 255, 255), 3);
        }
        return; // Retorna cedo para não desenhar círculos GT
    }
    
    // Draw ground truth (solid circles)
    cv::circle(image, gt.left_eye, 8, cv::Scalar(255, 100, 0), -1);  // Blue filled
    cv::circle(image, gt.left_eye, 10, cv::Scalar(255, 150, 0), 2);  // Blue outline
    cv::putText(image, "L-GT", cv::Point(gt.left_eye.x - 20, gt.left_eye.y - 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    
    cv::circle(image, gt.right_eye, 8, cv::Scalar(0, 255, 100), -1);  // Green filled
    cv::circle(image, gt.right_eye, 10, cv::Scalar(0, 255, 150), 2);  // Green outline
    cv::putText(image, "R-GT", cv::Point(gt.right_eye.x - 20, gt.right_eye.y - 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
    
    // Draw predictions if available
    if (predictions.find(frameNum) != predictions.end()) {
        const Prediction& pred = predictions[frameNum];
        
        // Left eye prediction (red dashed)
        cv::circle(image, pred.left_eye, 12, cv::Scalar(0, 0, 255), 3);
        cv::putText(image, "L-P", cv::Point(pred.left_eye.x + 15, pred.left_eye.y - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
        
        // Right eye prediction (yellow dashed)
        cv::circle(image, pred.right_eye, 12, cv::Scalar(0, 255, 255), 3);
        cv::putText(image, "R-P", cv::Point(pred.right_eye.x + 15, pred.right_eye.y - 15),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 2);
        
        // Draw error lines
        double leftError = calculateDistance(gt.left_eye, pred.left_eye);
        double rightError = calculateDistance(gt.right_eye, pred.right_eye);
        
        cv::line(image, gt.left_eye, pred.left_eye, cv::Scalar(0, 0, 255), 2);
        cv::line(image, gt.right_eye, pred.right_eye, cv::Scalar(0, 255, 255), 2);
        
        // Draw error text
        cv::Point leftMid((gt.left_eye.x + pred.left_eye.x) / 2, 
                         (gt.left_eye.y + pred.left_eye.y) / 2);
        cv::Point rightMid((gt.right_eye.x + pred.right_eye.x) / 2, 
                          (gt.right_eye.y + pred.right_eye.y) / 2);
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << leftError << "px";
        cv::putText(image, ss.str(), leftMid, cv::FONT_HERSHEY_SIMPLEX, 
                    0.5, cv::Scalar(255, 255, 255), 2);
        
        ss.str("");
        ss << std::fixed << std::setprecision(1) << rightError << "px";
        cv::putText(image, ss.str(), rightMid, cv::FONT_HERSHEY_SIMPLEX, 
                    0.5, cv::Scalar(255, 255, 255), 2);
    }
}

// Update display
void updateDisplay() {
    if (currentFrameImage.empty()) {
        return;
    }
    
    currentFrameImage.copyTo(displayImage);
    drawComparison(displayImage, currentFrame);
    
    // Draw overlay with info
    cv::Mat overlay;
    displayImage.copyTo(overlay);
    cv::rectangle(overlay, cv::Point(10, 10), cv::Point(400, 100), 
                  cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(overlay, 0.7, displayImage, 0.3, 0, displayImage);
    
    std::stringstream ss;
    ss << "Frame: " << currentFrame << " / " << (totalFrames - 1);
    cv::putText(displayImage, ss.str(), cv::Point(20, 35),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    bool hasGT = groundTruths.find(currentFrame) != groundTruths.end();
    bool hasPred = predictions.find(currentFrame) != predictions.end();
    
    std::string status;
    cv::Scalar color;
    if (hasGT && groundTruths[currentFrame].eyes_closed) {
        status = "Status: GT indicates EYES CLOSED";
        color = cv::Scalar(255, 255, 0); // Amarelo
    } else if (hasGT && hasPred) {
        status = "Status: GT + Prediction";
        color = cv::Scalar(0, 255, 0);
    } else if (hasGT) {
        status = "Status: GT only (no prediction)";
        color = cv::Scalar(255, 165, 0);
    } else if (hasPred) {
        status = "Status: Prediction only (no GT)";
        color = cv::Scalar(255, 100, 100);
    } else {
        status = "Status: No data";
        color = cv::Scalar(128, 128, 128);
    }
    
    cv::putText(displayImage, status, cv::Point(20, 65),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
    
    cv::imshow("Eye Tracking Test Comparison", displayImage);
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

// Main function
int main(int argc, char** argv) {
    std::cout << "\n=== Eye Tracking Test Framework ===\n\n";
    
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <video_file> <ground_truth.json> <predictions.csv>\n\n";
        std::cout << "Example:\n";
        std::cout << "  " << argv[0] << " test_video.mp4 ground_truth.json predictions.csv\n\n";
        return -1;
    }
    
    std::string videoPath = argv[1];
    std::string groundTruthPath = argv[2];
    std::string predictionsPath = argv[3];
    
    // Load data
    if (!loadGroundTruth(groundTruthPath)) {
        return -1;
    }
    
    if (!loadPredictions(predictionsPath)) {
        return -1;
    }
    
    // Open video
    videoCapture.open(videoPath);
    if (!videoCapture.isOpened()) {
        std::cerr << "Error: Could not open video file: " << videoPath << "\n";
        return -1;
    }
    
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    
    std::cout << "Video loaded: " << videoPath << "\n";
    std::cout << "Total frames: " << totalFrames << "\n\n";
    
    // Calculate metrics
    calculateMetrics();
    displayMetrics();
    
    // Save report
    std::string reportFile = "testing/test_reports/test_report.txt";
    saveMetricsReport(reportFile);
    
    // Create visualization window
    cv::namedWindow("Eye Tracking Test Comparison", CV_WINDOW_NORMAL);
    goToFrame(0);
    
    std::cout << "\nKeyboard controls:\n";
    std::cout << "  Arrow Keys / A,D  - Navigate frames\n";
    std::cout << "  PageUp/PageDown   - Skip 10 frames\n";
    std::cout << "  Q / ESC           - Quit\n\n";
    
    // Main loop
    bool running = true;
    while (running) {
        int key = cv::waitKey(10);
        
        switch (key) {
            case 'a':
            case 'A':
                goToFrame(currentFrame - 1);
                break;
                
            case 'd':
            case 'D':
                goToFrame(currentFrame + 1);
                break;
                
            case 'q':
            case 'Q':
            case 27: // ESC
                running = false;
                break;
        }
    }
    
    videoCapture.release();
    cv::destroyAllWindows();
    
    return 0;
}