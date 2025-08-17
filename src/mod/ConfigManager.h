#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ConfigManager
{
public:
    // Reads an openmw.cfg file
    bool load(const fs::path& config_path);
    // Writes the changes back to the file
    bool save(const fs::path& config_path);

    // Publicly accessible state
    std::vector<std::string> data_paths;
    std::vector<std::string> content_files;

private:
    std::vector<std::string> lines_before_data;
    std::vector<std::string> lines_between;
    std::vector<std::string> lines_after_content;
};
