#include "config.hpp"
#include <fstream>

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
