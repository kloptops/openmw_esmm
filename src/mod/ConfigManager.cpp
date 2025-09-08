#include "ConfigManager.h"
#include "ConfigParser.h" // NEW

#include <fstream>
#include <iostream>
#include <string>

bool ConfigManager::load(const fs::path& config_path) {
    m_source_path = config_path;
    auto result = ConfigParser::read_config(config_path);
    if (!result) return false;

    // Just copy the data, we don't hold it anymore
    m_loaded_data = *result;
    return true;
}

bool ConfigManager::save(const fs::path& target_path, const ConfigData& data_to_save) {
    return ConfigParser::write_config(target_path, m_source_path, data_to_save);
}
