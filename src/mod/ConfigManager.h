#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ConfigManager {
public:
    // Loads all data/content lines from the config.
    bool load(const fs::path& config_path);

    // Saves the new lists back to the config file, preserving other lines.
    bool save(const fs::path& config_path);

    // Public lists populated by load() and used by save().
    std::vector<std::string> data_paths;
    std::vector<std::string> content_files;
    std::vector<std::string> disabled_content_files;

private:
    // We only need to store the original lines of the file to reconstruct it on save.
    std::vector<std::string> m_original_lines;
};
