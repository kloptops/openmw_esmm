#include "ConfigParser.h"
#include "../utils/Utils.h"
#include <fstream>
#include <map>

// Helpers from old ConfigManager, now centralized here

static std::string extractAndCleanValue(const std::string& line, const std::string& prefix) {
    if (line.length() <= prefix.length()) return "";
    std::string value = line.substr(prefix.length());
    size_t comment_pos = value.find('#');
    if (comment_pos != std::string::npos) value = value.substr(0, comment_pos);
    value = trim(value);
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }
    return value;
}


std::unique_ptr<ConfigData> ConfigParser::read_config(const fs::path& cfg_path) {
    std::ifstream file(cfg_path.string());
    if (!file.is_open()) return nullptr; // Return null pointer on failure

    auto config_data = std::make_unique<ConfigData>();
    std::vector<std::string> enabled_content;
    std::vector<std::string> disabled_content;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed_line = trim(line);
        if (trimmed_line.rfind("data=", 0) == 0) {
            config_data->data_paths.emplace_back(extractAndCleanValue(line, "data="));
        } else if (trimmed_line.rfind("content=", 0) == 0) {
            enabled_content.push_back(extractAndCleanValue(line, "content="));
        } else if (trimmed_line.rfind("#content=", 0) == 0) {
            disabled_content.push_back(extractAndCleanValue(line, "#content="));
        }
    }
    
    for(const auto& name : enabled_content) config_data->content_files.push_back({name, true, "Unknown"});
    for(const auto& name : disabled_content) config_data->content_files.push_back({name, false, "Unknown"});

    return config_data;
}


bool ConfigParser::write_config(const fs::path& target_path, const fs::path& source_path, const ConfigData& data) {
    // 1. Read the entire source file into memory first.
    std::vector<std::string> source_lines;
    std::ifstream src_file(source_path.string());
    if (src_file.is_open()) {
        std::string line;
        while (std::getline(src_file, line)) {
            source_lines.push_back(line);
        }
        src_file.close();
    }
    // It's not an error if the source file doesn't exist; we'll create it.

    // 2. Now, open the target file for writing (which truncates it).
    std::ofstream dest_file(target_path.string());
    if (!dest_file.is_open()) {
        return false; // Cannot open for writing, this is a fatal error.
    }

    // 3. Iterate over the in-memory lines and write to the destination.
    bool data_written = false;
    bool content_written = false;

    for (const auto& line : source_lines) {
        std::string trimmed_line = trim(line);
        if (trimmed_line.rfind("data=", 0) == 0) {
            if (!data_written) {
                for (const auto& path : data.data_paths) {
                    dest_file << "data=" << path.string() << "\n";
                }
                data_written = true;
            }
        } else if (trimmed_line.rfind("content=", 0) == 0 || trimmed_line.rfind("#content=", 0) == 0) {
            if (!content_written) {
                for (const auto& cf : data.content_files) {
                    if (cf.enabled) dest_file << "content=" << cf.name << "\n";
                    else dest_file << "#content=" << cf.name << "\n";
                }
                content_written = true;
            }
        } else {
            dest_file << line << "\n";
        }
    }
    
    // If the source file was empty or didn't have data/content sections, write them now.
    if (!data_written) {
        for (const auto& path : data.data_paths) {
            dest_file << "data=" << path.string() << "\n";
        }
    }
    if (!content_written) {
        for (const auto& cf : data.content_files) {
            if (cf.enabled) dest_file << "content=" << cf.name << "\n";
            else dest_file << "#content=" << cf.name << "\n";
        }
    }

    return true;
}

