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
#include <algorithm>

#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL cv::WINDOW_NORMAL
#endif

// Structure for ground truth annotation
struct GroundTruth {
    cv::Point left_eye;
    cv::Point right_eye;
    bool eyes_closed;
    bool left_missing;
    bool right_missing;

    GroundTruth() : left_eye(-1, -1), right_eye(-1, -1), eyes_closed(false), left_missing(false), right_missing(false) {}
    GroundTruth(cv::Point l, cv::Point r, bool closed, bool l_miss, bool r_miss)
        : left_eye(l), right_eye(r), eyes_closed(closed), left_missing(l_miss), right_missing(r_miss) {}
};

// Structure for prediction
struct Prediction {
    cv::Point left_eye;
    cv::Point right_eye;
    Prediction() : left_eye(-1, -1), right_eye(-1, -1) {}
    Prediction(cv::Point l, cv::Point r) : left_eye(l), right_eye(r) {}
};

// Structure for metrics
struct TestMetrics {
    int total_annotated_frames;
    int closed_eyes_frames;
    int analyzed_left_eyes;
    int analyzed_right_eyes;

    std::vector<double> left_eye_errors, left_eye_errors_x, left_eye_errors_y;
    std::vector<double> right_eye_errors, right_eye_errors_x, right_eye_errors_y;
    std::vector<double> all_valid_errors;

    double avg_left, avg_left_x, avg_left_y;
    double avg_right, avg_right_x, avg_right_y;
    double avg_total;

    double rmse_left, rmse_left_x, rmse_left_y;
    double rmse_right, rmse_right_x, rmse_right_y;
    double rmse_total;

    double median_left, median_right, median_total;

    double max_left, max_left_x, max_left_y;
    double max_right, max_right_x, max_right_y;

    double std_left, std_right; 

    TestMetrics() : total_annotated_frames(0), closed_eyes_frames(0),
                    analyzed_left_eyes(0), analyzed_right_eyes(0),
                    avg_left(0), avg_left_x(0), avg_left_y(0),
                    avg_right(0), avg_right_x(0), avg_right_y(0), avg_total(0),
                    rmse_left(0), rmse_left_x(0), rmse_left_y(0),
                    rmse_right(0), rmse_right_x(0), rmse_right_y(0), rmse_total(0),
                    median_left(0), median_right(0), median_total(0),
                    max_left(0), max_left_x(0), max_left_y(0),
                    max_right(0), max_right_x(0), max_right_y(0),
                    std_left(0), std_right(0) {} // INICIALIZAÇÃO AQUI
};

// Global variables
std::map<int, GroundTruth> groundTruths;
std::map<int, Prediction> predictions;
cv::VideoCapture videoCapture;
int currentFrame = 0, totalFrames = 0;
double fps = 30.0; // RESTAURADO!
cv::Mat currentFrameImage, displayImage;
TestMetrics metrics;

// Function declarations
bool loadGroundTruth(const std::string& filename);
bool loadPredictions(const std::string& filename);
void calculateMetrics();
void displayMetrics(); // RESTAURADO!
void saveMetricsReport(const std::string& filename);
void saveCsvReport(const std::string& filename); // RESTAURADO!
void drawComparison(cv::Mat& image, int frameNum);
void goToFrame(int frameNum);
void updateDisplay();

// Math Helpers
double calculateDistance(const cv::Point& p1, const cv::Point& p2) { return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2)); }
double calculateDistanceX(const cv::Point& p1, const cv::Point& p2) { return std::abs(p1.x - p2.x); }
double calculateDistanceY(const cv::Point& p1, const cv::Point& p2) { return std::abs(p1.y - p2.y); }
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
    bool closed = false, l_miss = false, r_miss = false;

    while (std::getline(file, line)) {
        if (line.find("\"frame_number\":") != std::string::npos) {
            sscanf(line.c_str(), "      \"frame_number\": %d,", &frameNum);
            closed = false; l_miss = false; r_miss = false;
        }
        if (line.find("\"eyes_closed\":") != std::string::npos && line.find("true") != std::string::npos) closed = true;
        if (line.find("\"left_missing\":") != std::string::npos && line.find("true") != std::string::npos) l_miss = true;
        if (line.find("\"right_missing\":") != std::string::npos && line.find("true") != std::string::npos) r_miss = true;

        if (line.find("\"left_eye\":") != std::string::npos) sscanf(line.c_str(), "      \"left_eye\": {\"x\": %d, \"y\": %d},", &leftX, &leftY);
        if (line.find("\"right_eye\":") != std::string::npos) {
            sscanf(line.c_str(), "      \"right_eye\": {\"x\": %d, \"y\": %d}", &rightX, &rightY);
            if (frameNum >= 0) {
                groundTruths[frameNum] = GroundTruth(cv::Point(leftX, leftY), cv::Point(rightX, rightY), closed, l_miss, r_miss);
                frameNum = -1; leftX = leftY = rightX = rightY = -1;
            }
        }
    }
    file.close(); return true;
}

bool loadPredictions(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    predictions.clear();
    std::string line; std::getline(file, line);
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line); std::string token; std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) tokens.push_back(token);
        if (tokens.size() >= 9) {
            predictions[std::stoi(tokens[0])] = Prediction(cv::Point(std::stoi(tokens[7]), std::stoi(tokens[8])), cv::Point(std::stoi(tokens[5]), std::stoi(tokens[6])));
        }
    }
    file.close(); return true;
}

void calculateMetrics() {
    metrics = TestMetrics();
    metrics.total_annotated_frames = groundTruths.size();

    for (const auto& gtPair : groundTruths) {
        int frameNum = gtPair.first;
        const GroundTruth& gt = gtPair.second;

        if (predictions.find(frameNum) == predictions.end()) continue;
        if (gt.eyes_closed) { metrics.closed_eyes_frames++; continue; }

        const Prediction& pred = predictions[frameNum];

        // Avaliação Independente do Olho Esquerdo
        if (!gt.left_missing) {
            metrics.analyzed_left_eyes++;
            double err = calculateDistance(gt.left_eye, pred.left_eye);
            double errX = calculateDistanceX(gt.left_eye, pred.left_eye);
            double errY = calculateDistanceY(gt.left_eye, pred.left_eye);

            metrics.left_eye_errors.push_back(err);
            metrics.left_eye_errors_x.push_back(errX);
            metrics.left_eye_errors_y.push_back(errY);
            metrics.all_valid_errors.push_back(err);

            if (err > metrics.max_left) metrics.max_left = err;
            if (errX > metrics.max_left_x) metrics.max_left_x = errX;
            if (errY > metrics.max_left_y) metrics.max_left_y = errY;
        }

        // Avaliação Independente do Olho Direito
        if (!gt.right_missing) {
            metrics.analyzed_right_eyes++;
            double err = calculateDistance(gt.right_eye, pred.right_eye);
            double errX = calculateDistanceX(gt.right_eye, pred.right_eye);
            double errY = calculateDistanceY(gt.right_eye, pred.right_eye);

            metrics.right_eye_errors.push_back(err);
            metrics.right_eye_errors_x.push_back(errX);
            metrics.right_eye_errors_y.push_back(errY);
            metrics.all_valid_errors.push_back(err);

            if (err > metrics.max_right) metrics.max_right = err;
            if (errX > metrics.max_right_x) metrics.max_right_x = errX;
            if (errY > metrics.max_right_y) metrics.max_right_y = errY;
        }
    }

    // Matemática Segura
    int nL = metrics.left_eye_errors.size();
    if (nL > 0) {
        double sL=0, sLx=0, sLy=0, sqL=0, sqLx=0, sqLy=0;
        for (int i=0; i<nL; i++) {
            sL += metrics.left_eye_errors[i]; sqL += std::pow(metrics.left_eye_errors[i], 2);
            sLx += metrics.left_eye_errors_x[i]; sqLx += std::pow(metrics.left_eye_errors_x[i], 2);
            sLy += metrics.left_eye_errors_y[i]; sqLy += std::pow(metrics.left_eye_errors_y[i], 2);
        }
        metrics.avg_left = sL/nL; metrics.avg_left_x = sLx/nL; metrics.avg_left_y = sLy/nL;
        metrics.rmse_left = std::sqrt(sqL/nL); metrics.rmse_left_x = std::sqrt(sqLx/nL); metrics.rmse_left_y = std::sqrt(sqLy/nL);
        metrics.median_left = getMedian(metrics.left_eye_errors);
    }

    int nR = metrics.right_eye_errors.size();
    if (nR > 0) {
        double sR=0, sRx=0, sRy=0, sqR=0, sqRx=0, sqRy=0;
        for (int i=0; i<nR; i++) {
            sR += metrics.right_eye_errors[i]; sqR += std::pow(metrics.right_eye_errors[i], 2);
            sRx += metrics.right_eye_errors_x[i]; sqRx += std::pow(metrics.right_eye_errors_x[i], 2);
            sRy += metrics.right_eye_errors_y[i]; sqRy += std::pow(metrics.right_eye_errors_y[i], 2);
        }
        metrics.avg_right = sR/nR; metrics.avg_right_x = sRx/nR; metrics.avg_right_y = sRy/nR;
        metrics.rmse_right = std::sqrt(sqR/nR); metrics.rmse_right_x = std::sqrt(sqRx/nR); metrics.rmse_right_y = std::sqrt(sqRy/nR);
        metrics.median_right = getMedian(metrics.right_eye_errors);
    }
    // Standard Deviation
    if (nL > 0) {
        double varLeft = 0;
        for (int i=0; i<nL; i++) varLeft += std::pow(metrics.left_eye_errors[i] - metrics.avg_left, 2);
        metrics.std_left = std::sqrt(varLeft/nL);
    }
    if (nR > 0) {
        double varRight = 0;
        for (int i=0; i<nR; i++) varRight += std::pow(metrics.right_eye_errors[i] - metrics.avg_right, 2);
        metrics.std_right = std::sqrt(varRight/nR);
    }

    int nTotal = metrics.all_valid_errors.size();
    if (nTotal > 0) {
        double sTot=0, sqTot=0;
        for (int i=0; i<nTotal; i++) { sTot += metrics.all_valid_errors[i]; sqTot += std::pow(metrics.all_valid_errors[i], 2); }
        metrics.avg_total = sTot/nTotal;
        metrics.rmse_total = std::sqrt(sqTot/nTotal);
        metrics.median_total = getMedian(metrics.all_valid_errors);
    }
}

// RESTAURADO
void displayMetrics() {
    std::cout << "\n===============================================\n";
    std::cout << "           EYE TRACKING TEST RESULTS          \n";
    std::cout << "===============================================\n\n";
    std::cout << "Dataset Overview:\n";
    std::cout << "  Analyzed Left Eyes:  " << metrics.analyzed_left_eyes << "\n";
    std::cout << "  Analyzed Right Eyes: " << metrics.analyzed_right_eyes << "\n\n";
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
    file << "  Frames with Closed Eyes: " << metrics.closed_eyes_frames << " (Excluded)\n";
    file << "  Analyzed Left Eyes:      " << metrics.analyzed_left_eyes << " valid observations\n";
    file << "  Analyzed Right Eyes:     " << metrics.analyzed_right_eyes << " valid observations\n\n";

    file << "2. ERROR METRICS (PIXELS)\n-------------------------\n";
    file << std::fixed << std::setprecision(2);
    file << "[OVERALL PERFORMANCE (Across all valid eyes)]\n";
    file << "  RMSE:   " << metrics.rmse_total << " px\n";
    file << "  Mean:   " << metrics.avg_total << " px\n";
    file << "  Median: " << metrics.median_total << " px\n\n";

file << "[LEFT EYE DEEP DIVE]\n";
    file << "  Total Error -> RMSE: " << metrics.rmse_left << " | Mean: " << metrics.avg_left << " | Median: " << metrics.median_left << " | Max: " << metrics.max_left << " | StdDev: " << metrics.std_left << "\n";
    file << "  X-Axis Only -> RMSE: " << metrics.rmse_left_x << " | Mean: " << metrics.avg_left_x << " | Max: " << metrics.max_left_x << "\n";
    file << "  Y-Axis Only -> RMSE: " << metrics.rmse_left_y << " | Mean: " << metrics.avg_left_y << " | Max: " << metrics.max_left_y << "\n\n";

    file << "[RIGHT EYE DEEP DIVE]\n";
    file << "  Total Error -> RMSE: " << metrics.rmse_right << " | Mean: " << metrics.avg_right << " | Median: " << metrics.median_right << " | Max: " << metrics.max_right << " | StdDev: " << metrics.std_right << "\n";
    file << "  X-Axis Only -> RMSE: " << metrics.rmse_right_x << " | Mean: " << metrics.avg_right_x << " | Max: " << metrics.max_right_x << "\n";
    file << "  Y-Axis Only -> RMSE: " << metrics.rmse_right_y << " | Mean: " << metrics.avg_right_y << " | Max: " << metrics.max_right_y << "\n\n";
    file.close();
}

// RESTAURADO
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
            
            double lx = gt.left_missing ? 0 : calculateDistanceX(gt.left_eye, pred.left_eye);
            double ly = gt.left_missing ? 0 : calculateDistanceY(gt.left_eye, pred.left_eye);
            double l_total = gt.left_missing ? 0 : calculateDistance(gt.left_eye, pred.left_eye);
            
            double rx = gt.right_missing ? 0 : calculateDistanceX(gt.right_eye, pred.right_eye);
            double ry = gt.right_missing ? 0 : calculateDistanceY(gt.right_eye, pred.right_eye);
            double r_total = gt.right_missing ? 0 : calculateDistance(gt.right_eye, pred.right_eye);
            
            double avg_total = 0;
            if (!gt.left_missing && !gt.right_missing) avg_total = (l_total + r_total) / 2.0;
            else if (!gt.left_missing) avg_total = l_total;
            else if (!gt.right_missing) avg_total = r_total;

            file << std::fixed << std::setprecision(2);
            file << frameNum << ",OPEN,"
                 << lx << "," << ly << "," << l_total << ","
                 << rx << "," << ry << "," << r_total << ","
                 << avg_total << "\n";
        }
    }
    file.close();
}

void drawComparison(cv::Mat& image, int frameNum) {
    if (groundTruths.find(frameNum) == groundTruths.end()) return;
    const GroundTruth& gt = groundTruths[frameNum];

    if (gt.eyes_closed) {
        cv::putText(image, "GROUND TRUTH: EYES CLOSED", cv::Point(image.cols/2 - 200, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 3);
        return;
    }

    if (predictions.find(frameNum) != predictions.end()) {
        const Prediction& pred = predictions[frameNum];

        if (!gt.left_missing) {
            cv::circle(image, gt.left_eye, 8, cv::Scalar(255, 100, 0), -1);
            cv::circle(image, pred.left_eye, 12, cv::Scalar(0, 0, 255), 3);
            cv::line(image, gt.left_eye, pred.left_eye, cv::Scalar(0, 0, 255), 2);

            std::stringstream ss; ss << std::fixed << std::setprecision(1) << calculateDistance(gt.left_eye, pred.left_eye) << "px";
            cv::putText(image, ss.str(), cv::Point(pred.left_eye.x - 20, pred.left_eye.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
        } else {
            cv::putText(image, "L MISSING", cv::Point(pred.left_eye.x - 40, pred.left_eye.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(100, 100, 100), 2);
        }

        if (!gt.right_missing) {
            cv::circle(image, gt.right_eye, 8, cv::Scalar(0, 255, 100), -1);
            cv::circle(image, pred.right_eye, 12, cv::Scalar(0, 255, 255), 3);
            cv::line(image, gt.right_eye, pred.right_eye, cv::Scalar(0, 255, 255), 2);

            std::stringstream ss; ss << std::fixed << std::setprecision(1) << calculateDistance(gt.right_eye, pred.right_eye) << "px";
            cv::putText(image, ss.str(), cv::Point(pred.right_eye.x - 20, pred.right_eye.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2);
        } else {
            cv::putText(image, "R MISSING", cv::Point(pred.right_eye.x - 40, pred.right_eye.y - 20), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(100, 100, 100), 2);
        }
    }
}

void updateDisplay() {
    if (currentFrameImage.empty()) return;
    currentFrameImage.copyTo(displayImage);
    drawComparison(displayImage, currentFrame);

    cv::Mat overlay; displayImage.copyTo(overlay);
    
    // 1. Painel Superior (Frame Info & Teclas)
    cv::rectangle(overlay, cv::Point(10, 10), cv::Point(470, 110), cv::Scalar(0, 0, 0), -1);
    
    // 2. Painel Inferior (Dashboard de Métricas ao Vivo)
    cv::rectangle(overlay, cv::Point(10, displayImage.rows - 160), cv::Point(450, displayImage.rows - 10), cv::Scalar(30, 30, 30), -1);
    
    cv::addWeighted(overlay, 0.7, displayImage, 0.3, 0, displayImage);

    // --- TEXTO DO PAINEL SUPERIOR ---
    std::stringstream ss; 
    ss << "Frame: " << currentFrame << " / " << (totalFrames - 1);
    cv::putText(displayImage, ss.str(), cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    cv::putText(displayImage, "Controls:", cv::Point(20, 65), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
    cv::putText(displayImage, "[A/D] Prev/Next  |  [W/S] Jump 10 frames", cv::Point(100, 65), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
    cv::putText(displayImage, "[Q/ESC] Quit", cv::Point(100, 85), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);

    bool hasGT = groundTruths.find(currentFrame) != groundTruths.end();
    bool hasPred = predictions.find(currentFrame) != predictions.end();

    // --- TEXTO DO PAINEL INFERIOR (MÉTRICAS) ---
    if (hasGT && hasPred && !groundTruths[currentFrame].eyes_closed) {
        const GroundTruth& gt = groundTruths[currentFrame];
        const Prediction& pred = predictions[currentFrame];
        
        int y_pos = displayImage.rows - 130;
        cv::putText(displayImage, "CURRENT FRAME METRICS", cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 1);
        
        y_pos += 30;
        // Olho Esquerdo
        if (!gt.left_missing) {
            ss.str(""); ss << std::fixed << std::setprecision(1);
            ss << "LEFT EYE : " << calculateDistance(gt.left_eye, pred.left_eye) << "px "
               << "(X:" << calculateDistanceX(gt.left_eye, pred.left_eye) << " Y:" << calculateDistanceY(gt.left_eye, pred.left_eye) << ")";
            cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 200, 200), 1);
        } else {
            cv::putText(displayImage, "LEFT EYE : MISSING (Occluded)", cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(100, 100, 100), 1);
        }
        
        y_pos += 30;
        // Olho Direito
        if (!gt.right_missing) {
            ss.str(""); ss << std::fixed << std::setprecision(1);
            ss << "RIGHT EYE: " << calculateDistance(gt.right_eye, pred.right_eye) << "px "
               << "(X:" << calculateDistanceX(gt.right_eye, pred.right_eye) << " Y:" << calculateDistanceY(gt.right_eye, pred.right_eye) << ")";
            cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 255, 200), 1);
        } else {
            cv::putText(displayImage, "RIGHT EYE: MISSING (Occluded)", cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(100, 100, 100), 1);
        }
        
        y_pos += 30;
        // Erro Médio (Lidando com um ou os dois olhos ocultos)
        double l_total = gt.left_missing ? 0 : calculateDistance(gt.left_eye, pred.left_eye);
        double r_total = gt.right_missing ? 0 : calculateDistance(gt.right_eye, pred.right_eye);
        double avg_total = 0;
        
        if (!gt.left_missing && !gt.right_missing) avg_total = (l_total + r_total) / 2.0;
        else if (!gt.left_missing) avg_total = l_total;
        else if (!gt.right_missing) avg_total = r_total;

        if (!gt.left_missing || !gt.right_missing) {
            ss.str(""); ss << std::fixed << std::setprecision(1) << "AVG ERROR: " << avg_total << "px";
            // Pinta de vermelho se o erro da frame passar dos 15px
            cv::Scalar errColor = (avg_total > 15.0) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
            cv::putText(displayImage, ss.str(), cv::Point(20, y_pos), cv::FONT_HERSHEY_SIMPLEX, 0.6, errColor, 2);
        }
        
    } else if (hasGT && groundTruths[currentFrame].eyes_closed) {
        // Alerta de Olhos Fechados
        cv::putText(displayImage, "STATUS: EYES CLOSED (Skipped from calculations)", cv::Point(20, displayImage.rows - 70), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(100, 100, 255), 2);
    } else if (!hasGT) {
        cv::putText(displayImage, "NO GROUND TRUTH ANNOTATION", cv::Point(20, displayImage.rows - 70), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(100, 100, 100), 2);
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
    std::cout << "\n=== Eye Tracking Test Framework ===\n";
    
    // VERIFICAÇÃO COM AJUDA DETALHADA
    if (argc < 4) {
        std::cout << "Erro: Argumentos insuficientes.\n";
        std::cout << "Uso: " << argv[0] << " <video_file> <ground_truth.json> <predictions.csv>\n";
        std::cout << "Exemplo: " << argv[0] << " testing/test_data/test1.mov testing/test_data/gt.json testing/test_data/pred.csv\n\n";
        return -1;
    }
    
    std::string videoPath = argv[1];
    std::string groundTruthPath = argv[2];
    std::string predictionsPath = argv[3];
    
    std::cout << "Loading Ground Truth: " << groundTruthPath << "\n";
    if (!loadGroundTruth(groundTruthPath)) {
        std::cerr << "✗ Erro ao carregar Ground Truth.\n";
        return -1;
    }
    
    std::cout << "Loading Predictions: " << predictionsPath << "\n";
    if (!loadPredictions(predictionsPath)) {
        std::cerr << "✗ Erro ao carregar Predictions.\n";
        return -1;
    }
    
    videoCapture.open(videoPath);
    if (!videoCapture.isOpened()) { 
        std::cerr << "✗ Erro: Nao foi possivel abrir o video: " << videoPath << "\n"; 
        return -1; 
    }
    
    totalFrames = (int)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    fps = videoCapture.get(cv::CAP_PROP_FPS);
    
    std::cout << "Video loaded successfully. Total frames: " << totalFrames << "\n";
    
    calculateMetrics();
    displayMetrics();
    
    size_t lastSlash = videoPath.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? videoPath : videoPath.substr(lastSlash + 1);
    filename = filename.substr(0, filename.find_last_of('.'));
    
    std::string reportDir = "testing/test_reports";
    std::string reportTxt = reportDir + "/report_" + filename + ".txt";
    std::string reportCsv = reportDir + "/report_" + filename + ".csv";
    
    saveMetricsReport(reportTxt);
    saveCsvReport(reportCsv);
    
    cv::namedWindow("Eye Tracking Test Comparison", CV_WINDOW_NORMAL);
    goToFrame(0);
    
    // RESTAURAÇÃO DAS TECLAS DE NAVEGAÇÃO
    std::cout << "\nKeyboard controls:\n";
    std::cout << "  A / D             - Frame anterior / seguinte\n";
    std::cout << "  W / S             - Saltar 10 frames para tras / frente\n";
    std::cout << "  Q / ESC           - Sair\n\n";
    
    bool running = true;
    while (running) {
        int key = cv::waitKey(10);
        switch (key) {
            case 'a': case 'A': goToFrame(currentFrame - 1); break;
            case 'd': case 'D': goToFrame(currentFrame + 1); break;
            case 'w': case 'W': goToFrame(currentFrame - 10); break;
            case 's': case 'S': goToFrame(currentFrame + 10); break;
            case 'q': case 'Q': case 27: running = false; break;
        }
    }
    
    videoCapture.release(); 
    cv::destroyAllWindows();
    return 0;
}