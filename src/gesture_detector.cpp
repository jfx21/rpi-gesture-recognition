#include "gesture_detector.hpp"
#include <cmath>
#include <algorithm>

GestureDetector::GestureDetector() : channels_(3) {
    clahe_ = cv::createCLAHE(2.0, cv::Size(8, 8));
    morphKernel_ = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
}

double GestureDetector::calculateAngle(const cv::Point& s, const cv::Point& e, const cv::Point& f) {
    double a = cv::norm(e - s);
    double b = cv::norm(f - s);
    double c = cv::norm(f - e);
    if (b == 0.0 || c == 0.0) return 180.0;
    double cosAngle = (b * b + c * c - a * a) / (2.0 * b * c);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle));
    return std::acos(cosAngle) * 180.0 / CV_PI;
}

bool GestureDetector::checkMotion(const cv::Mat& frame) {
    cv::cvtColor(frame, gray_, cv::COLOR_BGR2GRAY);
    bool hasMotion = true;

    if (!prevGray_.empty()) {
        cv::absdiff(gray_, prevGray_, frameDiff_);
        cv::threshold(frameDiff_, frameDiff_, motionPixelDiff_, 255, cv::THRESH_BINARY);
        if (cv::countNonZero(frameDiff_) < motionMinPixels_) {
            hasMotion = false;
        }
    }
    gray_.copyTo(prevGray_);
    return hasMotion;
}

DetectionResult GestureDetector::process(cv::Mat& frame, cv::Mat& maskOut, const SkinThresholds& thresh, bool forceProcess) {
    DetectionResult result;
    result.fingerCount = lastFingerCount_;

    if (!forceProcess && !checkMotion(frame)) {
        result.processed = false;
        return result;
    }

    result.processed = true;

    // 1. Konwersja do YCrCb i CLAHE na kanale Y
    cv::cvtColor(frame, ycrcb_, cv::COLOR_BGR2YCrCb);
    cv::split(ycrcb_, channels_);
    clahe_->apply(channels_[0], channels_[0]);
    cv::merge(channels_, ycrcb_);

    // 2. Odszumianie, progowanie i morfologia
    cv::GaussianBlur(ycrcb_, ycrcb_, cv::Size(5, 5), 0);
    cv::Scalar lowerSkin(thresh.yMin, thresh.crMin, thresh.cbMin);
    cv::Scalar upperSkin(thresh.yMax, thresh.crMax, thresh.cbMax);
    cv::inRange(ycrcb_, lowerSkin, upperSkin, maskOut);
    cv::morphologyEx(maskOut, maskOut, cv::MORPH_OPEN, morphKernel_);
    cv::morphologyEx(maskOut, maskOut, cv::MORPH_CLOSE, morphKernel_);

    // 3. Najwiekszy kontur
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(maskOut, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

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

    // 4. Analiza wypuklosci i rozroznianie 0 vs 1 palec
    if (maxAreaIdx != -1) {
        const auto& handContour = contours[maxAreaIdx];
        std::vector<int> hullIndices;
        cv::convexHull(handContour, hullIndices, false);

        if (hullIndices.size() > 3) {
            std::vector<cv::Vec4i> defects;
            try {
                cv::convexityDefects(handContour, hullIndices, defects);
            } catch (const cv::Exception&) {
                defects.clear();
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

            if (validDefects > 0) {
                fingerCount = std::min(5, validDefects + 1);
            } else {
                std::vector<cv::Point> hullPoints;
                cv::convexHull(handContour, hullPoints, false);
                double hullArea = cv::contourArea(hullPoints);
                double solidity = (hullArea > 0.0) ? (maxArea / hullArea) : 1.0;
                double aspectRatio = static_cast<double>(boundRect.height) / boundRect.width;

                fingerCount = (aspectRatio > 1.35 || solidity < 0.80) ? 1 : 0;
            }
        }
    }

    lastFingerCount_ = fingerCount;
    result.fingerCount = fingerCount;
    return result;
}
