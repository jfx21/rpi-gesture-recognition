#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <cmath>
#include <vector>

double calculateAngle(const cv::Point& s, const cv::Point& e, const cv::Point& f) {
    double a = cv::norm(e - s);
    double b = cv::norm(f - s);
    double c = cv::norm(f - e);
    if (b == 0.0 || c == 0.0) return 180.0;
    double cosAngle = (b * b + c * c - a * a) / (2.0 * b * c);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle));
    return std::acos(cosAngle) * 180.0 / CV_PI;
}

int main() {
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

    cv::Mat frame, ycrcb, mask;
    std::vector<cv::Mat> channels(3);

    const cv::Scalar lowerSkin(20, 133, 77);
    const cv::Scalar upperSkin(255, 173, 127);

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
            const auto& handContour = contours[maxAreaIdx];
            std::vector<int> hullIndices;
            cv::convexHull(handContour, hullIndices, false);

            if (hullIndices.size() > 3) {
                std::vector<cv::Vec4i> defects;
                cv::convexityDefects(handContour, hullIndices, defects);

                cv::Rect boundRect = cv::boundingRect(handContour);
                int validDefects = 0;

                for (const auto& d : defects) {
                    cv::Point start = handContour[d[0]];
                    cv::Point end = handContour[d[1]];
                    cv::Point far = handContour[d[2]];
                    float depth = d[3] / 256.0f;

                    if (depth > boundRect.height * 0.15f && calculateAngle(start, end, far) < 90.0) {
                        validDefects++;
                    }
                }
                fingerCount = (validDefects > 0) ? std::min(5, validDefects + 1) : 0;
            }
        }

        auto tEnd = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

        std::cout << "Palce: " << fingerCount << " | Czas: " << elapsedMs << " ms   \r" << std::flush;

#ifndef HEADLESS
        cv::putText(frame, "Palce: " + std::to_string(fingerCount), cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Podglad", frame);
        cv::imshow("Maska", mask);
        if (cv::waitKey(1) == 27) {
            break;
        }
#endif
    }

    cap.release();
    return 0;
}
