#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ConfigManager {
public:
    bool load(const fs::path& config_path);
    bool save(const fs::path& config_path);

    // Lists to hold the final state for saving
    std::vector<std::string> base_data;
    std::vector<std::string> mod_data;
    std::vector<std::string> base_content;
    std::vector<std::string> mod_content;
    std::vector<std::string> disabled_content; // For #content= lines

private:
    // Buckets for all other lines to ensure we don't lose them
    std::vector<std::string> lines_before_base_data;
    std::vector<std::string> lines_between_data;
    std::vector<std::string> lines_between_content;
    std::vector<std::string> lines_after_mod_content;
};
