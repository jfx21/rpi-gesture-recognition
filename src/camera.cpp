#include "camera.hpp"
#include <iostream>

CameraStream::CameraStream(int deviceIndex, int width, int height, int fps)
    : deviceIndex_(deviceIndex), width_(width), height_(height), fps_(fps) {}

CameraStream::~CameraStream() {
    stop();
}

bool CameraStream::start() {
    cap_.open(deviceIndex_, cv::CAP_V4L2);
    if (!cap_.isOpened()) {
        std::cerr << "Blad: Nie mozna otworzyc kamery." << std::endl;
        return false;
    }

    cap_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);
    cap_.set(cv::CAP_PROP_FPS, fps_);

    running_ = true;
    worker_ = std::thread(&CameraStream::captureLoop, this);
    return true;
}

void CameraStream::stop() {
    if (running_) {
        running_ = false;
        if (worker_.joinable()) {
            worker_.join();
        }
    }
    if (cap_.isOpened()) {
        cap_.release();
    }
}

bool CameraStream::getLatestFrame(cv::Mat& outFrame) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!hasNewFrame_) return false;
    latestFrame_.copyTo(outFrame);
    hasNewFrame_ = false;
    return true;
}

void CameraStream::captureLoop() {
    cv::Mat temp;
    while (running_) {
        if (cap_.read(temp) && !temp.empty()) {
            std::lock_guard<std::mutex> lock(mtx_);
            temp.copyTo(latestFrame_);
            hasNewFrame_ = true;
        }
    }
}
