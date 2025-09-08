#pragma once
#include "ConfigParser.h"
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class ConfigManager {
public:
    bool load(const fs::path& config_path);
    bool save(const fs::path& target_path, const ConfigData& data_to_save);

    // Provide access to the loaded data
    const ConfigData& get_loaded_data() const { return m_loaded_data; }

private:
    fs::path m_source_path;
    ConfigData m_loaded_data;
};
