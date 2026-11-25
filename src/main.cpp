#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <vector>
#include <string>

struct SkinThresholds {
    int yMin = 20, yMax = 255;
    int crMin = 133, crMax = 173;
    int cbMin = 77, cbMax = 127;
};

const std::string CONFIG_FILE = "ycrcb_config.txt";

bool loadThresholds(const std::string& filename, SkinThresholds& t) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    file >> t.yMin >> t.yMax >> t.crMin >> t.crMax >> t.cbMin >> t.cbMax;
    return !file.fail();
}

bool saveThresholds(const std::string& filename, const SkinThresholds& t) {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    file << t.yMin << " " << t.yMax << " "
         << t.crMin << " " << t.crMax << " "
         << t.cbMin << " " << t.cbMax << "\n";
    return true;
}

double calculateAngle(const cv::Point& s, const cv::Point& e, const cv::Point& f) {
    double a = cv::norm(e - s);
    double b = cv::norm(f - s);
    double c = cv::norm(f - e);
    if (b == 0.0 || c == 0.0) return 180.0;
    double cosAngle = (b * b + c * c - a * a) / (2.0 * b * c);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle));
    return std::acos(cosAngle) * 180.0 / CV_PI;
}

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

    cv::VideoCapture cap(0, cv::CAP_V4L2);
    if (!cap.isOpened()) {
        std::cerr << "Blad: Nie mozna otworzyc kamery." << std::endl;
        return -1;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(cv::CAP_PROP_FPS, 30);

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    cv::Mat morphKernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));

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

    cv::Mat frame, ycrcb, mask;
    std::vector<cv::Mat> channels(3);

    while (true) {
        if (!cap.read(frame) || frame.empty()) {
            break;
        }

        auto tStart = std::chrono::high_resolution_clock::now();

        // Konwersja do YCrCb i korekcja jasnosci CLAHE
        cv::cvtColor(frame, ycrcb, cv::COLOR_BGR2YCrCb);
        cv::split(ycrcb, channels);
        clahe->apply(channels[0], channels[0]);
        cv::merge(channels, ycrcb);

        // Odszumianie, progowanie i morfologia
        cv::GaussianBlur(ycrcb, ycrcb, cv::Size(5, 5), 0);
        cv::Scalar lowerSkin(thresh.yMin, thresh.crMin, thresh.cbMin);
        cv::Scalar upperSkin(thresh.yMax, thresh.crMax, thresh.cbMax);
        cv::inRange(ycrcb, lowerSkin, upperSkin, mask);
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, morphKernel);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, morphKernel);

        // Najwiekszy kontur
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int fingerCount = 0;
        int maxAreaIdx = -1;
        double maxArea = 1500.0;

        for (size_t i = 0; i < contours.size(); i++) {
            double area = cv::contourArea(contours[i]);
            if (area > maxArea) {
                maxArea = area;
                maxAreaIdx = static_cast<int>(i);
            }
        }

        // Zliczanie palcow na podstawie defektow wypuklosci
        if (maxAreaIdx != -1) {
            // Uzywamy oryginalnego konturu (brak samoprzeciec po approxPolyDP)
            const auto& handContour = contours[maxAreaIdx];

            std::vector<int> hullIndices;
            cv::convexHull(handContour, hullIndices, false);

            if (hullIndices.size() > 3) {
                std::vector<cv::Vec4i> defects;
                try {
                    cv::convexityDefects(handContour, hullIndices, defects);
                } catch (const cv::Exception&) {
                    defects.clear(); // Pominiecie uszkodzonej klatki zamiast crashu
                }

                cv::Rect boundRect = cv::boundingRect(handContour);
                int validDefects = 0;

#ifndef HEADLESS
                cv::rectangle(frame, boundRect, cv::Scalar(255, 0, 0), 1);
                cv::drawContours(frame, std::vector<std::vector<cv::Point>>{handContour}, -1, cv::Scalar(0, 255, 0), 1);
#endif

                for (const auto& d : defects) {
                    cv::Point start = handContour[d[0]];
                    cv::Point end = handContour[d[1]];
                    cv::Point far = handContour[d[2]];
                    float depth = d[3] / 256.0f;

                    bool isAboveWrist = far.y < (boundRect.y + boundRect.height * 0.75);

                    if (isAboveWrist &&
                        depth > boundRect.height * 0.12f &&
                        depth < boundRect.height * 0.60f &&
                        calculateAngle(start, end, far) < 85.0) {
                        validDefects++;
#ifndef HEADLESS
                        cv::circle(frame, far, 4, cv::Scalar(0, 0, 255), -1);
#endif
                    }
                }
                fingerCount = (validDefects > 0) ? std::min(5, validDefects + 1) : 0;
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        if (!calibrateMode) {
            std::cout << "Palce: " << fingerCount << " | Czas: " << elapsedMs << " ms   \r" << std::flush;
        }

#ifndef HEADLESS
        cv::putText(frame, "Palce: " + std::to_string(fingerCount), cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Podglad", frame);
        cv::imshow("Maska", mask);

        int key = cv::waitKey(1);
        if (key == 27) { // ESC
            break;
        } else if (calibrateMode && (key == 's' || key == 'S')) {
            if (saveThresholds(CONFIG_FILE, thresh)) {
                std::cout << "Zapisano progi do " << CONFIG_FILE << ": "
                          << thresh.yMin << " " << thresh.yMax << " "
                          << thresh.crMin << " " << thresh.crMax << " "
                          << thresh.cbMin << " " << thresh.cbMax << std::endl;
            } else {
                std::cerr << "Blad zapisu do pliku " << CONFIG_FILE << std::endl;
            }
        }
#endif
    }

    cap.release();
    return 0;
}