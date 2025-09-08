#pragma once
#include <vector>
#include <string>
#include <memory>
#include <boost/filesystem.hpp>
#include "ModManager.h"

namespace fs = boost::filesystem;

struct ConfigData {
    std::vector<fs::path> data_paths;
    std::vector<ContentFile> content_files;
};

class ConfigParser {
public:
    static std::unique_ptr<ConfigData> read_config(const fs::path& cfg_path);
    
    static bool write_config(
        const fs::path& target_path,
        const fs::path& source_path,
        const ConfigData& data
    );
};
