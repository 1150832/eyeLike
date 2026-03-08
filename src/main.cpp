#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <stdio.h>
#include <math.h>

#include "constants.h"
#include "findEyeCenter.h"
#include "findEyeCorner.h"
#include "MqttPublisher.h"

/* Attempt at supporting openCV version 4.0.1 or higher */
#if CV_MAJOR_VERSION >= 4
#define CV_WINDOW_NORMAL                cv::WINDOW_NORMAL
#define CV_BGR2YCrCb                    cv::COLOR_BGR2YCrCb
#define CV_HAAR_SCALE_IMAGE             cv::CASCADE_SCALE_IMAGE
#define CV_HAAR_FIND_BIGGEST_OBJECT     cv::CASCADE_FIND_BIGGEST_OBJECT
#endif


/** Constants **/


/** Function Headers */
void detectAndDisplay( cv::Mat frame );
void printUsage();

/** Global variables */
//-- Note, either copy these two files from opencv/data/haarscascades to your current folder, or change these locations
cv::String face_cascade_name = "res/haarcascade_frontalface_alt.xml";
cv::String eye_cascade_name = "res/haarcascade_eye_tree_eyeglasses.xml";
cv::CascadeClassifier face_cascade;
cv::CascadeClassifier eye_cascade;
std::string main_window_name = "Capture - Face detection";
std::string face_window_name = "Capture - Face";
cv::RNG rng(12345);
cv::Mat debugImage;
cv::Mat skinCrCbHist = cv::Mat::zeros(cv::Size(256, 256), CV_8UC1);

// MQTT variables
MqttPublisher mqttPublisher;
bool useMqtt = false;
std::string mqttBroker = "tcp://localhost:1883";
std::string mqttTopic = "eyetracker/coordinates";
std::string mqttClientId = "eyeLike";

MqttMode mqttMode = MqttMode::PRODUCTION;

// Variables for video manipulation & Execution Mode
std::ofstream outputFile;
bool saveToFile = false;
int globalFrameNumber = 0;
bool headlessMode = false; // NOVO: Flag para modo sem interface gráfica

/**
 * @function printUsage
 */
void printUsage() {
  std::cout << "\n=== eyeLike - Eye Tracking Application ===\n\n";
  std::cout << "Usage: eyeLike [options]\n\n";
  std::cout << "Options:\n";
  std::cout << "  (no arguments)          Run with webcam (default mode)\n";
  std::cout << "  -c, --camera            Run with webcam\n";
  std::cout << "  -v, --video <path>      Process video file\n";
  std::cout << "  -o, --output <path>     Save results to CSV file\n";
  std::cout << "  -m, --mqtt <broker>     Enable MQTT publishing (default: tcp://localhost:1883)\n";
  std::cout << "  -t, --topic <topic>     MQTT topic (default: eyetracker/coordinates)\n";
  std::cout << "  --client-id <id>        MQTT client ID (default: eyeLike)\n";
  std::cout << "  --mqtt-mode <mode>      MQTT mode: production, debug, heartbeat (default: production)\n";
  std::cout << "  --headless              Run without GUI/Video output (for Raspberry Pi/Servers)\n"; // NOVO
  std::cout << "  -h, --help              Show this help message\n\n";
  std::cout << "Examples:\n";
  std::cout << "  eyeLike                          # Use webcam\n";
  std::cout << "  eyeLike -v video.mp4 --headless  # Process video without windows\n";
  std::cout << "  eyeLike -c -m tcp://localhost:1883 --headless # Webcam to MQTT in background\n\n";
  std::cout << "Keyboard Controls (GUI Mode Only):\n";
  std::cout << "  'c' or 'q'  - Quit application\n";
  std::cout << "  'f'         - Save current frame as image\n";
  std::cout << "  'p'         - Pause video (video mode only)\n\n";
}

/**
 * @function main
 */
int main( int argc, const char** argv ) {
  cv::Mat frame;
  
  // Variáveis para parsing de argumentos
  std::string videoPath = "";
  std::string outputPath = "";
  bool useCamera = true;
  
  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    
    if (arg == "-h" || arg == "--help") {
      printUsage();
      return 0;
    }
    else if (arg == "-c" || arg == "--camera") {
      useCamera = true;
    }
    else if (arg == "-v" || arg == "--video") {
      if (i + 1 < argc) {
        videoPath = argv[++i];
        useCamera = false;
      } else {
        std::cerr << "Error: --video requires a path argument\n";
        printUsage();
        return -1;
      }
    }
    else if (arg == "-o" || arg == "--output") {
      if (i + 1 < argc) {
        outputPath = argv[++i];
        saveToFile = true;
      } else {
        std::cerr << "Error: --output requires a path argument\n";
        printUsage();
        return -1;
      }
    }
    else if (arg == "-m" || arg == "--mqtt") {
      if (i + 1 < argc) {
        mqttBroker = argv[++i];
        useMqtt = true;
      } else {
        std::cerr << "Error: --mqtt requires a broker URL\n";
        printUsage();
        return -1;
      }
    }
    else if (arg == "-t" || arg == "--topic") {
      if (i + 1 < argc) {
        mqttTopic = argv[++i];
      } else {
        std::cerr << "Error: --topic requires a topic name\n";
        printUsage();
        return -1;
      }
    }
    else if (arg == "--client-id") {
      if (i + 1 < argc) {
        mqttClientId = argv[++i];
      } else {
        std::cerr << "Error: --client-id requires an ID\n";
        printUsage();
        return -1;
      }
    }
    else if (arg == "--mqtt-mode") {
      if (i + 1 < argc) {
        std::string modeStr = argv[++i];
        if (modeStr == "production") mqttMode = MqttMode::PRODUCTION;
        else if (modeStr == "debug") mqttMode = MqttMode::DEBUG;
        else if (modeStr == "heartbeat") mqttMode = MqttMode::HEARTBEAT;
        else {
          std::cerr << "Unknown MQTT mode: " << modeStr << "\n";
          printUsage();
          return -1;
        } 
      }
    }
    else if (arg == "--headless") {
      headlessMode = true; // NOVO: Ativa o modo headless
    }
    else {
      std::cerr << "Unknown argument: " << arg << "\n";
      printUsage();
      return -1;
    }
  }

  // Load the cascades
  if( !face_cascade.load( face_cascade_name ) ) { 
    printf("--(!)Error loading face cascade, please change face_cascade_name in source code.\n"); 
    return -1; 
  }
  
  if( !eye_cascade.load( eye_cascade_name ) ) { 
    printf("--(!)Error loading eye cascade, please change eye_cascade_name in source code.\n"); 
    return -1; 
  }
  
  // Initialize MQTT if enabled
  if (useMqtt) {
    std::cout << "\n=== MQTT Configuration ===\n";
    std::cout << "Broker: " << mqttBroker << "\n";
    std::cout << "Topic: " << mqttTopic << "\n";
    std::cout << "Client ID: " << mqttClientId << "\n";
    
      if (!mqttPublisher.connect(mqttBroker, mqttClientId, mqttTopic, mqttMode)) {
        std::cerr << "Warning: MQTT connection failed. Continuing without MQTT.\n";
        useMqtt = false;
      }
  }

  // Open output file if specified
  if (saveToFile) {
    outputFile.open(outputPath.c_str());
    if (!outputFile.is_open()) {
      std::cerr << "Error: Could not open output file: " << outputPath << "\n";
      return -1;
    }
    // Write CSV header
    outputFile << "frame,face_x,face_y,face_width,face_height,";
    outputFile << "right_eye_x,right_eye_y,left_eye_x,left_eye_y,";
    outputFile << "single_eye_x,single_eye_y,global_eye_x,global_eye_y\n";
    std::cout << "Saving results to: " << outputPath << "\n";
  }
  
  // Ignores opening windows if in headless mode
  if (!headlessMode) {
      cv::namedWindow(main_window_name, CV_WINDOW_NORMAL);
      cv::moveWindow(main_window_name, 400, 100);
      cv::namedWindow(face_window_name, CV_WINDOW_NORMAL);
      cv::moveWindow(face_window_name, 10, 100);
      cv::namedWindow("Right Eye", CV_WINDOW_NORMAL);
      cv::moveWindow("Right Eye", 10, 600);
      cv::namedWindow("Left Eye", CV_WINDOW_NORMAL);
      cv::moveWindow("Left Eye", 10, 800);
  }

  createCornerKernels();
  ellipse(skinCrCbHist, cv::Point(113, 155), cv::Size(23, 15),
          43.0, 0.0, 360.0, cv::Scalar(255, 255, 255), -1);

  // Informação sobre modo de operação
  if (headlessMode) {
      std::cout << "\n>>> RUNNING IN HEADLESS MODE (No GUI) <<<\n";
      if (useCamera) std::cout << "Press Ctrl+C in the terminal to stop the application.\n";
  }

  if (useCamera) {
    std::cout << "Starting in CAMERA mode...\n";
  } else {
    std::cout << "Starting in VIDEO mode: " << videoPath << "\n";
  }

  // I make an attempt at supporting both 2.x and 3.x OpenCV
#if CV_MAJOR_VERSION < 3
  CvCapture* capture;
  
  // Open video source based on mode
  if (useCamera) {
    capture = cvCaptureFromCAM(0);
  } else {
    capture = cvCaptureFromFile(videoPath.c_str());
  }
  
  if( capture ) {
    // Get video properties for video files
    if (!useCamera) {
      double fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
      int totalFrames = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
      std::cout << "Video properties:\n";
      std::cout << "  FPS: " << fps << "\n";
      std::cout << "  Total frames: " << totalFrames << "\n\n";
    }
    
    while( true ) {
      frame = cvQueryFrame( capture );
#else
  cv::VideoCapture capture;
  
  // Open video source based on mode
  if (useCamera) {
    capture.open(0);
  } else {
    capture.open(videoPath);
  }
  
  if( capture.isOpened() ) {
    // Get video properties for video files
    if (!useCamera) {
      double fps = capture.get(cv::CAP_PROP_FPS);
      int totalFrames = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
      std::cout << "Video properties:\n";
      std::cout << "  FPS: " << fps << "\n";
      std::cout << "  Total frames: " << totalFrames << "\n\n";
    }
    
    while( true ) {
      capture.read(frame);
#endif
      
      // Check if frame is valid
      if( frame.empty() ) {
        if (!useCamera) {
          std::cout << "\nEnd of video reached.\n";
          std::cout << "Processed " << globalFrameNumber << " frames total.\n";
        } else {
          printf(" --(!) No captured frame -- Break!");
        }
        break;
      }
      
      globalFrameNumber++;
      
      // mirror it (only for camera mode)
      if (useCamera) {
        cv::flip(frame, frame, 1);
      }
      
      frame.copyTo(debugImage);

      // Apply the classifier to the frame
      if( !frame.empty() ) {
        detectAndDisplay( frame );
      }
      else {
        printf(" --(!) No captured frame -- Break!");
        break;
      }

      // Show progress for video files
      if (!useCamera && globalFrameNumber % 30 == 0) {
#if CV_MAJOR_VERSION < 3
        int totalFrames = (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
#else
        int totalFrames = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
#endif
        if (totalFrames > 0) {
          double progress = (100.0 * globalFrameNumber) / totalFrames;
          std::cout << "Processing: frame " << globalFrameNumber << " / " << totalFrames 
                    << " (" << progress << "%)\n";
        }
      }

      // NOVO: Ignora a visualização e captura de teclas no modo headless
      if (!headlessMode) {
          imshow(main_window_name,debugImage);

          // Different wait time for video vs camera
          int waitTime = useCamera ? 10 : 1;
          int c = cv::waitKey(waitTime);
          
          if( (char)c == 'c' || (char)c == 'q' ) { 
            std::cout << "\nExiting...\n";
            break; 
          }
          if( (char)c == 'f' ) {
            std::string filename = "captures/frame_" + std::to_string(globalFrameNumber) + ".png";
            imwrite(filename, frame);
            std::cout << "Saved frame to: " << filename << "\n";
          }
          if( (char)c == 'p' && !useCamera ) {
            std::cout << "Video PAUSED. Press any key to continue...\n";
            cv::waitKey(0);
            std::cout << "Resuming...\n";
          }
      }

    }
  }
  else {
    std::cerr << "Error: Could not open video source\n";
    if (saveToFile) outputFile.close();
    return -1;
  }

  // Cleanup
#if CV_MAJOR_VERSION < 3
  cvReleaseCapture(&capture);
#else
  capture.release();
#endif

  if (saveToFile) {
    outputFile.close();
    std::cout << "\nResults saved to: " << outputPath << "\n";
  }

  // Cleanup MQTT
  if (useMqtt) {
    mqttPublisher.disconnect();
  }

  releaseCornerKernels();

  return 0;
}

cv::Rect findEyes(cv::Mat frame_gray, cv::Rect face) {
  cv::Mat faceROI = frame_gray(face);
  cv::Mat debugFace = faceROI;

  if (kSmoothFaceImage) {
    double sigma = kSmoothFaceFactor * face.width;
    GaussianBlur( faceROI, faceROI, cv::Size( 0, 0 ), sigma);
  }
  //-- Find eye regions and draw them
  int eye_region_width = face.width * (kEyePercentWidth/100.0);
  int eye_region_height = face.width * (kEyePercentHeight/100.0);
  int eye_region_top = face.height * (kEyePercentTop/100.0);
  cv::Rect leftEyeRegion(face.width*(kEyePercentSide/100.0),
                         eye_region_top,eye_region_width,eye_region_height);
  cv::Rect rightEyeRegion(face.width - eye_region_width - face.width*(kEyePercentSide/100.0),
                          eye_region_top,eye_region_width,eye_region_height);

  //-- Find Eye Centers (com validação de olhos fechados)
  std::vector<cv::Rect> leftEyesDetected, rightEyesDetected;
  
  eye_cascade.detectMultiScale(faceROI(leftEyeRegion), leftEyesDetected, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(15, 15));
  eye_cascade.detectMultiScale(faceROI(rightEyeRegion), rightEyesDetected, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(15, 15));

  bool leftEyeClosed = leftEyesDetected.empty();
  bool rightEyeClosed = rightEyesDetected.empty();

  cv::Point leftPupil, rightPupil;

  if (!leftEyeClosed) {
      leftPupil = findEyeCenter(faceROI, leftEyeRegion, "Left Eye");
  } else {
      leftPupil = cv::Point(leftEyeRegion.width / 2, leftEyeRegion.height / 2);
  }

  if (!rightEyeClosed) {
      rightPupil = findEyeCenter(faceROI, rightEyeRegion, "Right Eye");
  } else {
      rightPupil = cv::Point(rightEyeRegion.width / 2, rightEyeRegion.height / 2);
  }

  // get corner regions
  cv::Rect leftRightCornerRegion(leftEyeRegion);
  leftRightCornerRegion.width -= leftPupil.x;
  leftRightCornerRegion.x += leftPupil.x;
  leftRightCornerRegion.height /= 2;
  leftRightCornerRegion.y += leftRightCornerRegion.height / 2;
  cv::Rect leftLeftCornerRegion(leftEyeRegion);
  leftLeftCornerRegion.width = leftPupil.x;
  leftLeftCornerRegion.height /= 2;
  leftLeftCornerRegion.y += leftLeftCornerRegion.height / 2;
  cv::Rect rightLeftCornerRegion(rightEyeRegion);
  rightLeftCornerRegion.width = rightPupil.x;
  rightLeftCornerRegion.height /= 2;
  rightLeftCornerRegion.y += rightLeftCornerRegion.height / 2;
  cv::Rect rightRightCornerRegion(rightEyeRegion);
  rightRightCornerRegion.width -= rightPupil.x;
  rightRightCornerRegion.x += rightPupil.x;
  rightRightCornerRegion.height /= 2;
  rightRightCornerRegion.y += rightRightCornerRegion.height / 2;
  rectangle(debugFace,leftRightCornerRegion,200);
  rectangle(debugFace,leftLeftCornerRegion,200);
  rectangle(debugFace,rightLeftCornerRegion,200);
  rectangle(debugFace,rightRightCornerRegion,200);
  // change eye centers to face coordinates
  rightPupil.x += rightEyeRegion.x;
  rightPupil.y += rightEyeRegion.y;
  leftPupil.x += leftEyeRegion.x;
  leftPupil.y += leftEyeRegion.y;

  cv::Rect singleEye;
  singleEye.x = (rightPupil.x+leftPupil.x) / 2.;
  singleEye.y = (rightPupil.y+leftPupil.y) / 2.; 
  singleEye.width  = rightPupil.x-leftPupil.x;
  singleEye.height = rightPupil.y-leftPupil.y;

  // draw eye centers
  circle(debugFace, rightPupil, 3, 1234);
  circle(debugFace, leftPupil, 3, 1234);
  
  // Apenas imprimir logs de debug se NÃO estivermos no modo headless, para manter o terminal do RPi limpo
  if (!headlessMode) {
      std::cout << "  RE: " << rightPupil.x << ", " << rightPupil.y << "\n";
      std::cout << "  LE: " << leftPupil.x  << ", " << leftPupil.y  << "\n";
  }
  
  // Save to CSV file if enabled
  if (saveToFile) {
    outputFile << globalFrameNumber << ",";
    outputFile << face.x << "," << face.y << "," << face.width << "," << face.height << ",";
    outputFile << (rightPupil.x + face.x) << "," << (rightPupil.y + face.y) << ",";
    outputFile << (leftPupil.x + face.x) << "," << (leftPupil.y + face.y) << ",";
    outputFile << singleEye.x << "," << singleEye.y << ",";
    outputFile << (singleEye.x + face.x) << "," << (singleEye.y + face.y) << "\n";
  }

  // Publish to MQTT if enabled
  if (useMqtt) {
    mqttPublisher.publishEyeData(
      globalFrameNumber,
      face.x, face.y, face.width, face.height,
      rightPupil.x + face.x, rightPupil.y + face.y,  // Add face.x, face.y
      leftPupil.x + face.x, leftPupil.y + face.y     // Add face.x, face.y
    );
  }

  //-- Find Eye Corners
  if (kEnableEyeCorner) {
    cv::Point2f leftRightCorner = findEyeCorner(faceROI(leftRightCornerRegion), true, false);
    leftRightCorner.x += leftRightCornerRegion.x;
    leftRightCorner.y += leftRightCornerRegion.y;
    cv::Point2f leftLeftCorner = findEyeCorner(faceROI(leftLeftCornerRegion), true, true);
    leftLeftCorner.x += leftLeftCornerRegion.x;
    leftLeftCorner.y += leftLeftCornerRegion.y;
    cv::Point2f rightLeftCorner = findEyeCorner(faceROI(rightLeftCornerRegion), false, true);
    rightLeftCorner.x += rightLeftCornerRegion.x;
    rightLeftCorner.y += rightLeftCornerRegion.y;
    cv::Point2f rightRightCorner = findEyeCorner(faceROI(rightRightCornerRegion), false, false);
    rightRightCorner.x += rightRightCornerRegion.x;
    rightRightCorner.y += rightRightCornerRegion.y;
    circle(faceROI, leftRightCorner, 3, 200);
    circle(faceROI, leftLeftCorner, 3, 200);
    circle(faceROI, rightLeftCorner, 3, 200);
    circle(faceROI, rightRightCorner, 3, 200);
  }

  // NOVO: Ignora a atualização da janela do rosto no modo headless
  if (!headlessMode) {
      imshow(face_window_name, faceROI);
  }

  return singleEye;
}


cv::Mat findSkin (cv::Mat &frame) {
  cv::Mat input;
  cv::Mat output = cv::Mat(frame.rows,frame.cols, CV_8U);

  cvtColor(frame, input, CV_BGR2YCrCb);

  for (int y = 0; y < input.rows; ++y) {
    const cv::Vec3b *Mr = input.ptr<cv::Vec3b>(y);
//    uchar *Or = output.ptr<uchar>(y);
    cv::Vec3b *Or = frame.ptr<cv::Vec3b>(y);
    for (int x = 0; x < input.cols; ++x) {
      cv::Vec3b ycrcb = Mr[x];
//      Or[x] = (skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) > 0) ? 255 : 0;
      if(skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) == 0) {
        Or[x] = cv::Vec3b(0,0,0);
      }
    }
  }
  return output;
}


/**
 * @function findHeads
 */
std::vector<cv::Rect> findHeads(cv::Mat frame_gray) {

  std::vector<cv::Rect> faces;

  //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT, cv::Size(150, 150) );

  //-- Show faces found
  for( int i = 0; i < faces.size(); i++ ) {
    rectangle(debugImage, faces[i], 1234);
    line(debugImage, cv::Point(faces[i].x, faces[i].y),
                cv::Point(faces[i].x+faces[i].width, faces[i].y+faces[i].height), 1234);
    line(debugImage, cv::Point(faces[i].x, faces[i].y+faces[i].height),
                cv::Point(faces[i].x+faces[i].width, faces[i].y), 1234);
  }

  //-- Return all face
  return faces;
}

/**
 * @function findBestHead
 */
cv::Rect findBestHead(cv::Mat frame_gray) {

  //-- Detect faces
  std::vector<cv::Rect> faces = findHeads(frame_gray);

  static cv::Rect bestFace = cv::Rect(frame_gray.cols/2-75, frame_gray.rows/2-75, 150, 150);

  if ( faces.size()>0 )
	bestFace = faces[0];

  //-- Show Best face found
  for( int i = 0; i < faces.size(); i++ ) {
    rectangle(debugImage, bestFace, 8080);
  }

  //-- Return best face
  return bestFace;
}


/**
 * @function detectAndDisplay
 */
void detectAndDisplay( cv::Mat frame ) {
  std::vector<cv::Rect> faces;
  cv::Mat frame_gray;

  std::vector<cv::Mat> rgbChannels(3);
  cv::split(frame, rgbChannels);

  cvtColor( frame, frame_gray, cv::COLOR_BGR2GRAY );

  faces = findHeads(frame_gray);
  //-- Show what you got
  if (faces.size() > 0) {
    
    // Prints face information only if not in headless mode, to avoid cluttering the terminal on Raspberry Pi
    if (!headlessMode) {
        std::cout << "Face: " << faces[0].x << ", " << faces[0].y << "; Size: " << 
	    faces[0].width << ", " << faces[0].height << "\n";
    }
    
    cv::Rect se = findEyes(frame_gray, faces[0]);
    
    if (!headlessMode) {
        std::cout << "  singleEye: " << se.x << ", " << se.y << "\n";
        std::cout << "  globalEye: " << se.x+faces[0].x << ", " << se.y+faces[0].y << "\n";
    }
  }
}