#include "Utils.h"
#include "../AppContext.h"

#include <cstdlib>
#include <regex>
#include <algorithm>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;


std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}


std::string getEnvVar(const std::string& name, const std::string& default_val)
{
    const char* value = std::getenv(name.c_str());
    return value ? value : default_val;
}


// Re-usable from our previous work
std::string cleanModName(const std::string& filename) {
    fs::path p(filename);
    std::string name = p.stem().string();
    std::regex version_pattern1(R"((.+?)\s+-\s*\d+)");
    std::regex version_pattern2(R"((.+?)-\d{4,})");
    std::smatch match;

    if (std::regex_search(name, match, version_pattern1) || std::regex_search(name, match, version_pattern2)) {
        if (match.size() > 1) name = match[1].str();
    }

    name = std::regex_replace(name, std::regex(R"(\s*\([^)]*\))"), "");
    name = std::regex_replace(name, std::regex(R"([\s-.]+$)"), "");
    
    return name;
}

// New utility to get just the version part
std::string extractVersionString(const std::string& filename, const std::string& clean_name) {
    fs::path p(filename);
    std::string stem = p.stem().string();
    
    // Find the starting position of the version info by looking for where the clean name ends
    size_t pos = stem.find(clean_name);
    if (pos != std::string::npos) {
        std::string version_part = stem.substr(pos + clean_name.length());
        // Clean up leading characters like '-' or spaces
        version_part = std::regex_replace(version_part, std::regex("^(\\s*-\\s*|\\s+)"), "");
        if (!version_part.empty()) {
            return version_part;
        }
    }

    return "N/A";
}
