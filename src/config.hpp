#pragma once
#include <string>

struct SkinThresholds {
    int yMin = 20, yMax = 255;
    int crMin = 133, crMax = 173;
    int cbMin = 77, cbMax = 127;
};

bool loadThresholds(const std::string& filename, SkinThresholds& t);
bool saveThresholds(const std::string& filename, const SkinThresholds& t);
