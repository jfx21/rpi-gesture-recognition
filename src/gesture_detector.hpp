#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "config.hpp"

struct DetectionResult {
    int fingerCount = 0;
    bool processed = false; // false oznacza pominiecie klatki przez Motion Gating
};

class GestureDetector {
public:
    GestureDetector();
    DetectionResult process(cv::Mat& frame, cv::Mat& maskOut, const SkinThresholds& thresh, bool forceProcess);

private:
    static double calculateAngle(const cv::Point& s, const cv::Point& e, const cv::Point& f);
    bool checkMotion(const cv::Mat& frame);

    cv::Ptr<cv::CLAHE> clahe_;
    cv::Mat morphKernel_;
    cv::Mat ycrcb_, gray_, prevGray_, frameDiff_;
    std::vector<cv::Mat> channels_;

    int lastFingerCount_ = 0;
    const int motionPixelDiff_ = 25;
    const int motionMinPixels_ = 250;
};
