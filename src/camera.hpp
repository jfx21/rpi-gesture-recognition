#pragma once
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>

class CameraStream {
public:
    CameraStream(int deviceIndex = 0, int width = 320, int height = 240, int fps = 30);
    ~CameraStream();

    bool start();
    void stop();
    bool getLatestFrame(cv::Mat& outFrame);

private:
    void captureLoop();

    int deviceIndex_, width_, height_, fps_;
    cv::VideoCapture cap_;
    cv::Mat latestFrame_;
    std::mutex mtx_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    bool hasNewFrame_{false};
};
