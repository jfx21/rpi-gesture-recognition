#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include "config.hpp"
#include "camera.hpp"
#include "gesture_detector.hpp"

const std::string CONFIG_FILE = "ycrcb_config.txt";

int main(int argc, char** argv) {
    bool calibrateMode = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--calibrate") {
            calibrateMode = true;
        }
    }

#ifdef HEADLESS
    if (calibrateMode) {
        std::cerr << "Blad: Tryb --calibrate wymaga kompilacji z GUI (HEADLESS=OFF)." << std::endl;
        return -1;
    }
#endif

    SkinThresholds thresh;
    if (loadThresholds(CONFIG_FILE, thresh)) {
        std::cout << "Wczytano progi z " << CONFIG_FILE << std::endl;
    } else {
        std::cout << "Brak pliku " << CONFIG_FILE << " - uzywam progow domyslnych." << std::endl;
    }

    CameraStream camera(0, 320, 240, 30);
    if (!camera.start()) {
        return -1;
    }

    GestureDetector detector;

#ifndef HEADLESS
    if (calibrateMode) {
        cv::namedWindow("Kalibracja YCrCb", cv::WINDOW_AUTOSIZE);
        cv::createTrackbar("Y min",  "Kalibracja YCrCb", &thresh.yMin,  255);
        cv::createTrackbar("Y max",  "Kalibracja YCrCb", &thresh.yMax,  255);
        cv::createTrackbar("Cr min", "Kalibracja YCrCb", &thresh.crMin, 255);
        cv::createTrackbar("Cr max", "Kalibracja YCrCb", &thresh.crMax, 255);
        cv::createTrackbar("Cb min", "Kalibracja YCrCb", &thresh.cbMin, 255);
        cv::createTrackbar("Cb max", "Kalibracja YCrCb", &thresh.cbMax, 255);
        std::cout << "Tryb kalibracji: nacisnij [s] aby zapisac progi, [ESC] aby wyjsc." << std::endl;
    }
#endif

    cv::Mat frame, mask;
    bool running = true;

    while (running) {
        if (!camera.getLatestFrame(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        auto tStart = std::chrono::high_resolution_clock::now();
        DetectionResult res = detector.process(frame, mask, thresh, calibrateMode);
        auto tEnd = std::chrono::high_resolution_clock::now();

        double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        if (!calibrateMode) {
            std::cout << "Palce: " << res.fingerCount
                      << " | Czas: " << elapsedMs << " ms"
                      << (res.processed ? " [PROC]" : " [SKIP]")
                      << "   \r" << std::flush;
        }

#ifndef HEADLESS
        std::string statusText = "Palce: " + std::to_string(res.fingerCount) + (res.processed ? "" : " (IDLE)");
        cv::putText(frame, statusText, cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Podglad", frame);
        if (!mask.empty()) {
            cv::imshow("Maska", mask);
        }

        int key = cv::waitKey(1);
        if (key == 27) {
            running = false;
        } else if (calibrateMode && (key == 's' || key == 'S')) {
            if (saveThresholds(CONFIG_FILE, thresh)) {
                std::cout << "Zapisano progi do " << CONFIG_FILE << std::endl;
            }
        }
#endif
    }

    camera.stop();
    return 0;
}
