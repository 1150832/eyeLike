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
#include <algorithm> // Necessário para calcular a Mediana

/* Support for OpenCV 4.x */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#endif

// Structure for ground truth annotation
struct GroundTruth {
    cv::Point left_eye;
    cv::Point right_eye;
    bool eyes_closed;
    
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
    int closed_eyes_frames;
    
    std::vector<double> left_eye_errors, left_eye_errors_x, left_eye_errors_y;
    std::vector<double> right_eye_errors, right_eye_errors_x, right_eye_errors_y;
    std::vector<double> avg_errors;
    
    // Mean (Averages)
    double avg_left, avg_left_x, avg_left_y;
    double avg_right, avg_right_x, avg_right_y;
    double avg_total;
    
    // RMSE (Root Mean Square Error) - Standard in scientific papers
    double rmse_left, rmse_left_x, rmse_left_y;
    double rmse_right, rmse_right_x, rmse_right_y;
    double rmse_total;
    
    // Median (Robust to outliers)
    double median_left, median_right, median_total;
    
    // Extremes
    double max_left, max_left_x, max_left_y;
    double max_right, max_right_x, max_right_y;
    
    double std_left, std_right;
    
    int within_5px, within_10px, within_20px, within_50px;
    
    TestMetrics() : total_annotated_frames(0), frames_with_predictions(0), closed_eyes_frames(0),
                    avg_left(0), avg_left_x(0), avg_left_y(0),
                    avg_right(0), avg_right_x(0), avg_right_y(0), avg_total(0),
                    rmse_left(0), rmse_left_x(0), rmse_left_y(0),
                    rmse_right(0), rmse_right_x(0), rmse_right_y(0), rmse_total(0),
                    median_left(0), median_right(0), median_total(0),
                    max_left(0), max_left_x(0), max_left_y(0),
                    max_right(0), max_right_x(0), max_right_y(0),
                    std_left(0), std_right(0),
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
void saveCsvReport(const std::string& filename);
void drawComparison(cv::Mat& image, int frameNum);
void goToFrame(int frameNum);
void updateDisplay();

double calculateDistance(const cv::Point& p1, const cv::Point& p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}
double calculateDistanceX(const cv::Point& p1, const cv::Point& p2) {
    return std::abs(p1.x - p2.x);
}
double calculateDistanceY(const cv::Point& p1, const cv::Point& p2) {
    return std::abs(p1.y - p2.y);
}

// Function to calculate Median safely
double getMedian(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    if (v.size() % 2 == 0) return (v[v.size() / 2 - 1] + v[v.size() / 2]) / 2.0;
    return v[v.size() / 2];
}

bool loadGroundTruth(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    groundTruths.clear();
    std::string line;
    int frameNum = -1, leftX = -1, leftY = -1, rightX = -1, rightY = -1;
    bool closed = false; 
    
    while (std::getline(file, line)) {
        if (line.find("\"frame_number\":") != std::string::npos) {
            sscanf(line.c_str(), "      \"frame_number\": %d,", &frameNum);
            closed = false; 
        }
        if (line.find("\"eyes_closed\":") != std::string::npos) {
            if (line.find("true") != std::string::npos) closed = true;
        }
        if (line.find("\"left_eye\":") != std::string::npos) {
            sscanf(line.c_str(), "      \"left_eye\": {\"x\": %d, \"y\": %d},", &leftX, &leftY);
        }
        if (line.find("\"right_eye\":") != std::string::npos) {
            sscanf(line.c_str(), "      \"right_eye\": {\"x\": %d, \"y\": %d}", &rightX, &rightY);
            if (frameNum >= 0 && (closed || (leftX >= 0 && rightX >= 0))) {
                groundTruths[frameNum] = GroundTruth(cv::Point(leftX, leftY), cv::Point(rightX, rightY), closed);
                frameNum = -1; leftX = leftY = rightX = rightY = -1; closed = false;
            }
        }
    }
    file.close();
    return true;
}

bool loadPredictions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    predictions.clear();
    std::string line;
    std::getline(file, line); 
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) tokens.push_back(token);
        
        if (tokens.size() >= 9) {
            int frameNum = std::stoi(tokens[0]);
            int rightX = std::stoi(tokens[5]);
            int rightY = std::stoi(tokens[6]);
            int leftX = std::stoi(tokens[7]);
            int leftY = std::stoi(tokens[8]);
            predictions[frameNum] = Prediction(cv::Point(leftX, leftY), cv::Point(rightX, rightY));
        }
    }
    file.close();
    return true;
}

void calculateMetrics() {
    metrics = TestMetrics();
    metrics.total_annotated_frames = groundTruths.size();
    
    for (const auto& gtPair : groundTruths) {
        int frameNum = gtPair.first;
        const GroundTruth& gt = gtPair.second;
        
        if (predictions.find(frameNum) == predictions.end()) continue;
        if (gt.eyes_closed) {
            metrics.closed_eyes_frames++;
            continue; 
        }
        
        const Prediction& pred = predictions[frameNum];
        metrics.frames_with_predictions++;
        
        double lErr = calculateDistance(gt.left_eye, pred.left_eye);
        double lErrX = calculateDistanceX(gt.left_eye, pred.left_eye);
        double lErrY = calculateDistanceY(gt.left_eye, pred.left_eye);
        
        double rErr = calculateDistance(gt.right_eye, pred.right_eye);
        double rErrX = calculateDistanceX(gt.right_eye, pred.right_eye);
        double rErrY = calculateDistanceY(gt.right_eye, pred.right_eye);
        
        double avgErr = (lErr + rErr) / 2.0;
        
        metrics.left_eye_errors.push_back(lErr);
        metrics.left_eye_errors_x.push_back(lErrX);
        metrics.left_eye_errors_y.push_back(lErrY);
        
        metrics.right_eye_errors.push_back(rErr);
        metrics.right_eye_errors_x.push_back(rErrX);
        metrics.right_eye_errors_y.push_back(rErrY);
        
        metrics.avg_errors.push_back(avgErr);
        
        // Update Max
        if (lErr > metrics.max_left) metrics.max_left = lErr;
        if (lErrX > metrics.max_left_x) metrics.max_left_x = lErrX;
        if (lErrY > metrics.max_left_y) metrics.max_left_y = lErrY;
        
        if (rErr > metrics.max_right) metrics.max_right = rErr;
        if (rErrX > metrics.max_right_x) metrics.max_right_x = rErrX;
        if (rErrY > metrics.max_right_y) metrics.max_right_y = rErrY;
        
        // Thresholds
        if (avgErr <= 5.0) metrics.within_5px++;
        if (avgErr <= 10.0) metrics.within_10px++;
        if (avgErr <= 20.0) metrics.within_20px++;
        if (avgErr <= 50.0) metrics.within_50px++;
    }
    
    if (!metrics.left_eye_errors.empty()) {
        int n = metrics.left_eye_errors.size();
        double sumL=0, sumLx=0, sumLy=0, sqSumL=0, sqSumLx=0, sqSumLy=0;
        double sumR=0, sumRx=0, sumRy=0, sqSumR=0, sqSumRx=0, sqSumRy=0;
        double sumAvg=0, sqSumAvg=0;
        
        for (int i = 0; i < n; i++) {
            sumL += metrics.left_eye_errors[i]; sqSumL += std::pow(metrics.left_eye_errors[i], 2);
            sumLx += metrics.left_eye_errors_x[i]; sqSumLx += std::pow(metrics.left_eye_errors_x[i], 2);
            sumLy += metrics.left_eye_errors_y[i]; sqSumLy += std::pow(metrics.left_eye_errors_y[i], 2);
            
            sumR += metrics.right_eye_errors[i]; sqSumR += std::pow(metrics.right_eye_errors[i], 2);
            sumRx += metrics.right_eye_errors_x[i]; sqSumRx += std::pow(metrics.right_eye_errors_x[i], 2);
            sumRy += metrics.right_eye_errors_y[i]; sqSumRy += std::pow(metrics.right_eye_errors_y[i], 2);
            
            sumAvg += metrics.avg_errors[i]; sqSumAvg += std::pow(metrics.avg_errors[i], 2);
        }
        
        // Means
        metrics.avg_left = sumL / n; metrics.avg_left_x = sumLx / n; metrics.avg_left_y = sumLy / n;
        metrics.avg_right = sumR / n; metrics.avg_right_x = sumRx / n; metrics.avg_right_y = sumRy / n;
        metrics.avg_total = sumAvg / n;
        
        // RMSE
        metrics.rmse_left = std::sqrt(sqSumL / n); metrics.rmse_left_x = std::sqrt(sqSumLx / n); metrics.rmse_left_y = std::sqrt(sqSumLy / n);
        metrics.rmse_right = std::sqrt(sqSumR / n); metrics.rmse_right_x = std::sqrt(sqSumRx / n); metrics.rmse_right_y = std::sqrt(sqSumRy / n);
        metrics.rmse_total = std::sqrt(sqSumAvg / n);
        
        // Medians
        metrics.median_left = getMedian(metrics.left_eye_errors);
        metrics.median_right = getMedian(metrics.right_eye_errors);
        metrics.median_total = getMedian(metrics.avg_errors);
        
        // Std Dev
        double varLeft = 0, varRight = 0;
        for (int i = 0; i < n; i++) {
            varLeft += std::pow(metrics.left_eye_errors[i] - metrics.avg_left, 2);
            varRight += std::pow(metrics.right_eye_errors[i] - metrics.avg_right, 2);
        }
        metrics.std_left = std::sqrt(varLeft / n);
        metrics.std_right = std::sqrt(varRight / n);
    }
}

void displayMetrics() {
    std::cout << "\n===============================================\n";
    std::cout << "           EYE TRACKING TEST RESULTS          \n";
    std::cout << "===============================================\n\n";
    std::cout << "Dataset Overview:\n";
    std::cout << "  Analyzed Frames (Open Eyes): " << metrics.frames_with_predictions << "\n\n";
    std::cout << "Error Summary (pixels):\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Overall RMSE:   " << metrics.rmse_total << " px\n";
    std::cout << "  Overall Mean:   " << metrics.avg_total << " px\n";
    std::cout << "  Overall Median: " << metrics.median_total << " px\n\n";
}

void saveMetricsReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "EYE TRACKING SCIENTIFIC REPORT\n===============================\n\n";
    file << "1. DATASET OVERVIEW\n-------------------\n";
    file << "  Total Annotated Frames:  " << metrics.total_annotated_frames << "\n";
    file << "  Frames with Closed Eyes: " << metrics.closed_eyes_frames << " (Excluded from tracking error)\n";
    file << "  Analyzed Frames (Open):  " << metrics.frames_with_predictions << "\n\n";
    
    file << "2. ERROR METRICS (PIXELS)\n-------------------------\n";
    file << std::fixed << std::setprecision(2);
    
    file << "[OVERALL PERFORMANCE]\n";
    file << "  Root Mean Square Error (RMSE): " << metrics.rmse_total << " px\n";
    file << "  Mean Absolute Error (MAE):     " << metrics.avg_total << " px\n";
    file << "  Median Error (Robust):         " << metrics.median_total << " px\n\n";
    
    file << "[LEFT EYE DEEP DIVE]\n";
    file << "  Total Error -> RMSE: " << metrics.rmse_left << " | Mean: " << metrics.avg_left << " | Median: " << metrics.median_left << " | Max: " << metrics.max_left << "\n";
    file << "  X-Axis Only -> RMSE: " << metrics.rmse_left_x << " | Mean: " << metrics.avg_left_x << " | Max: " << metrics.max_left_x << "\n";
    file << "  Y-Axis Only -> RMSE: " << metrics.rmse_left_y << " | Mean: " << metrics.avg_left_y << " | Max: " << metrics.max_left_y << "\n\n";
    
    file << "[RIGHT EYE DEEP DIVE]\n";
    file << "  Total Error -> RMSE: " << metrics.rmse_right << " | Mean: " << metrics.avg_right << " | Median: " << metrics.median_right << " | Max: " << metrics.max_right << "\n";
    file << "  X-Axis Only -> RMSE: " << metrics.rmse_right_x << " | Mean: " << metrics.avg_right_x << " | Max: " << metrics.max_right_x << "\n";
    file << "  Y-Axis Only -> RMSE: " << metrics.rmse_right_y << " | Mean: " << metrics.avg_right_y << " | Max: " << metrics.max_right_y << "\n\n";
    
    int n = metrics.frames_with_predictions;
    if (n > 0) {
        file << "3. ACCURACY THRESHOLDS\n----------------------\n";
        file << "  <  5px error: " << metrics.within_5px << " frames (" << (100.0 * metrics.within_5px / n) << "%)\n";
        file << "  < 10px error: " << metrics.within_10px << " frames (" << (100.0 * metrics.within_10px / n) << "%)\n";
        file << "  < 20px error: " << metrics.within_20px << " frames (" << (100.0 * metrics.within_20px / n) << "%)\n";
        file << "  < 50px error: " << metrics.within_50px << " frames (" << (100.0 * metrics.within_50px / n) << "%)\n";
    }
    file.close();
}

void saveCsvReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    file << "frame,state,left_err_x,left_err_y,left_err_total,right_err_x,right_err_y,right_err_total,avg_err_total\n";
    for (const auto& gtPair : groundTruths) {
        int frameNum = gtPair.first;
        const GroundTruth& gt = gtPair.second;
        if (gt.eyes_closed) {
             file << frameNum << ",CLOSED,0,0,0,0,0,0,0\n";
        } else if (predictions.find(frameNum) != predictions.end()) {
            const Prediction& pred = predictions[frameNum];
            file << std::fixed << std::setprecision(2);
            file << frameNum << ",OPEN," 
                 << calculateDistanceX(gt.left_eye, pred.left_eye) << "," << calculateDistanceY(gt.left_eye, pred.left_eye) << "," << calculateDistance(gt.left_eye, pred.left_eye) << ","
                 << calculateDistanceX(gt.right_eye, pred.right_eye) << "," << calculateDistanceY(gt.right_eye, pred.right_eye) << "," << calculateDistance(gt.right_eye, pred.right_eye) << ","
                 << ((calculateDistance(gt.left_eye, pred.left_eye) + calculateDistance(gt.right_eye, pred.right_eye)) / 2.0) << "\n";
        }
    }
    file.close();
}

// RESTAURAÇÃO: Voltar a mostrar os pixeis na imagem e criar Dashboard
void drawComparison(cv::Mat& image, int frameNum) {
    if (groundTruths.find(frameNum) == groundTruths.end()) return;
    const GroundTruth& gt = groundTruths[frameNum];
    
    if (gt.eyes_closed) {
        cv::putText(image, "GROUND TRUTH: EYES CLOSED", cv::Point(image.cols / 2 - 200, 100),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 3);
        if (predictions.find(frameNum) != predictions.end()) {
            const Prediction& pred = predictions[frameNum];
            cv::circle(image, pred.left_eye, 12, cv::Scalar(0, 0, 255), 3);
            cv::circle(image, pred.right_eye, 12, cv::Scalar(0, 255, 255), 3);
        }
        return; 
    }
    
    // Desenhar Ground Truth
    cv::circle(image, gt.left_eye, 8, cv::Scalar(255, 100, 0), -1);  
    cv::circle(image, gt.right_eye, 8, cv::Scalar(0, 255, 100), -1);  
    
    if (predictions.find(frameNum) != predictions.end()) {
        const Prediction& pred = predictions[frameNum];
        
        // Desenhar Predição
        cv::circle(image, pred.left_eye, 12, cv::Scalar(0, 0, 255), 3);
        cv::circle(image, pred.right_eye, 12, cv::Scalar(0, 255, 255), 3);
        
        // Desenhar Linhas de Distância
        cv::line(image, gt.left_eye, pred.left_eye, cv::Scalar(0, 0, 255), 2);
        cv::line(image, gt.right_eye, pred.right_eye, cv::Scalar(0, 255, 255), 2);
        
        // RECUPERAÇÃO: Desenhar Texto da Distância Exata junto à linha
        double lErr = calculateDistance(gt.left_eye, pred.left_eye);
        double rErr = calculateDistance(gt.right_eye, pred.right_eye);
        
        cv::Point leftMid((gt.left_eye.x + pred.left_eye.x)/2, (gt.left_eye.y + pred.left_eye.y)/2);
        cv::Point rightMid((gt.right_eye.x + pred.right_eye.x)/2, (gt.right_eye.y + pred.right_eye.y)/2);
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << lErr << "px";
        cv::putText(image, ss.str(), cv::Point(leftMid.x - 20, leftMid.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
        
        ss.str("");
        ss << std::fixed << std::setprecision(1) << rErr << "px";
        cv::putText(image, ss.str(), cv::Point(rightMid.x - 20, rightMid.y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
    }
}

void updateDisplay() {
    if (currentFrameImage.empty()) return;
    currentFrameImage.copyTo(displayImage);
    drawComparison(displayImage, currentFrame);
    
    // 1. Painel Superior (Estado Geral)
    cv::Mat overlay;
    displayImage.copyTo(overlay);
    cv::rectangle(overlay, cv::Point(10, 10), cv::Point(350, 80), cv::Scalar(0, 0, 0), -1);
    
    // 2. NOVO: Dashboard do Frame Atual (Lado Esquerdo Inferior)
    cv::rectangle(overlay, cv::Point(10, displayImage.rows - 160), cv::Point(350, displayImage.rows - 10), cv::Scalar(30, 30, 30), -1);
    cv::addWeighted(overlay, 0.7, displayImage, 0.3, 0, displayImage);
    
    // Texto do Painel Superior
    std::stringstream ss;
    ss << "Frame: " << currentFrame << " / " << (totalFrames - 1);
    cv::putText(displayImage, ss.str(), cv::Point(20, 35), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    bool hasGT = groundTruths.find(currentFrame) != groundTruths.end();
    bool hasPred = predictions.find(currentFrame) != predictions.end();
    std::string status = "No data"; cv::Scalar color = cv::Scalar(128, 128, 128);
    
    if (hasGT && groundTruths[currentFrame].eyes_closed) { status = "GT: EYES CLOSED"; color = cv::Scalar(255, 255, 0); }
    else if (hasGT && hasPred) { status = "Status: Evaluating"; color = cv::Scalar(0, 255, 0); }
    
    cv::putText(displayImage, status, cv::Point(20, 65), cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
    
    // Texto do Dashboard Inferior (Se tivermos ambas as anotações e estiverem de olhos abertos)
    if (hasGT && hasPred && !groundTruths[currentFrame].eyes_closed) {
        const GroundTruth& gt = groundTruths[currentFrame];
        const Prediction& pred = predictions[currentFrame];
        
        int y_pos = displayImage.rows - 130;
        cv::putText(displayImage, "CURRENT FRAME METRICS", cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 1);
        
        y_pos += 30;
        ss.str(""); ss << std::fixed << std::setprecision(1);
        ss << "LEFT EYE : " << calculateDistance(gt.left_eye, pred.left_eye) << "px "
           << "(X:" << calculateDistanceX(gt.left_eye, pred.left_eye) << " Y:" << calculateDistanceY(gt.left_eye, pred.left_eye) << ")";
        cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 200, 200), 1);
        
        y_pos += 30;
        ss.str(""); ss << std::fixed << std::setprecision(1);
        ss << "RIGHT EYE: " << calculateDistance(gt.right_eye, pred.right_eye) << "px "
           << "(X:" << calculateDistanceX(gt.right_eye, pred.right_eye) << " Y:" << calculateDistanceY(gt.right_eye, pred.right_eye) << ")";
        cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 255, 200), 1);
        
        y_pos += 30;
        double avg = (calculateDistance(gt.left_eye, pred.left_eye) + calculateDistance(gt.right_eye, pred.right_eye)) / 2.0;
        ss.str(""); ss << std::fixed << std::setprecision(1) << "AVG ERROR: " << avg << "px";
        
        // Pinta de vermelho se o erro for superior a 15px
        cv::Scalar errColor = (avg > 15.0) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
        cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.6, errColor, 2);
    }
    
    cv::imshow("Eye Tracking Test Comparison", displayImage);
}

void goToFrame(int frameNum) {
    if (frameNum < 0) frameNum = 0;
    if (frameNum >= totalFrames) frameNum = totalFrames - 1;
    videoCapture.set(cv::CAP_PROP_POS_FRAMES, frameNum);
    videoCapture.read(currentFrameImage);
    if (!currentFrameImage.empty()) { currentFrame = frameNum; updateDisplay(); }
}

int main(int argc, char** argv) {
    std::cout << "\n=== Eye Tracking Test Framework ===\n\n";
    if (argc < 4) {
        std::cout << "Usage: " << argv[0] << " <video_file> <ground_truth.json> <predictions.csv>\n";
        return -1;
    }
    
    std::string videoPath = argv[1];
    std::string groundTruthPath = argv[2];
    std::string predictionsPath = argv[3];
    
    if (!loadGroundTruth(groundTruthPath) || !loadPredictions(predictionsPath)) return -1;
    
    videoCapture.open(videoPath);
    if (!videoCapture.isOpened()) { std::cerr << "Error: Could not open video file.\n"; return -1; }
    
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    
    calculateMetrics();
    displayMetrics();
    
    size_t lastSlash = videoPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? videoPath : videoPath.substr(lastSlash + 1);
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) filename = filename.substr(0, lastDot);
    
    std::string reportDir = "testing/test_reports";
    std::string reportTxt = reportDir + "/report_" + filename + ".txt";
    std::string reportCsv = reportDir + "/report_" + filename + ".csv";
    
    saveMetricsReport(reportTxt);
    saveCsvReport(reportCsv);
    
    cv::namedWindow("Eye Tracking Test Comparison", CV_WINDOW_NORMAL);
    goToFrame(0);
    
    std::cout << "\nKeyboard controls:\n  Arrow Keys / A,D  - Navigate frames\n  Q / ESC           - Quit\n\n";
    
    bool running = true;
    while (running) {
        int key = cv::waitKey(10);
        switch (key) {
            case 'a': case 'A': goToFrame(currentFrame - 1); break;
            case 'd': case 'D': goToFrame(currentFrame + 1); break;
            case 'q': case 'Q': case 27: running = false; break;
        }
    }
    
    videoCapture.release();
    cv::destroyAllWindows();
    return 0;
}