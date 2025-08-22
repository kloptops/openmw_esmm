#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <string>

// Helper to check if a line is a data or content line
static bool isDataLine(const std::string& line) {
    return line.rfind("data=", 0) == 0;
}

static bool isContentLine(const std::string& line) {
    return line.rfind("content=", 0) == 0 || line.rfind("#content=", 0) == 0;
}

// --- NEW HELPER FUNCTION ---
// Extracts the value after a prefix, removes comments, trims whitespace, and removes surrounding quotes.
static std::string extractAndCleanValue(const std::string& line, const std::string& prefix) {
    if (line.length() <= prefix.length()) {
        return "";
    }
    std::string value = line.substr(prefix.length());

    // 1. Remove comments
    size_t comment_pos = value.find('#');
    if (comment_pos != std::string::npos) {
        value = value.substr(0, comment_pos);
    }

    // 2. Trim whitespace
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);

    // 3. Remove quotes if they exist on both ends
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.length() - 2);
    }

    return value;
}


bool ConfigManager::load(const fs::path& config_path) {
    // Clear all previous data
    data_paths.clear();
    content_files.clear();
    disabled_content_files.clear();
    m_original_lines.clear();

    std::ifstream file(config_path.string());
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        m_original_lines.push_back(line); // Store every line for the save operation

        // --- UPDATED LOGIC ---
        // Use the helper to correctly parse the value
        if (line.rfind("data=", 0) == 0) {
            data_paths.push_back(extractAndCleanValue(line, "data="));
        } else if (line.rfind("content=", 0) == 0) {
            content_files.push_back(extractAndCleanValue(line, "content="));
        } else if (line.rfind("#content=", 0) == 0) {
            disabled_content_files.push_back(extractAndCleanValue(line, "#content="));
        }
    }
    return true;
}

bool ConfigManager::save(const fs::path& config_path) {
    std::ofstream file(config_path.string());
    if (!file.is_open()) return false;

    bool data_written = false;
    bool content_written = false;

    for (const auto& line : m_original_lines) {
        if (isDataLine(line)) {
            if (!data_written) {
                // This is the first data line we've seen. Write our new list.
                for (const auto& p : data_paths) {
                    file << "data=" << p << '\n';
                }
                data_written = true;
            }
            // And now we skip this original line, since we've replaced the whole block.
        } else if (isContentLine(line)) {
            if (!content_written) {
                // This is the first content line. Write our new lists.
                for (const auto& c : content_files) {
                    file << "content=" << c << '\n';
                }
                for (const auto& c : disabled_content_files) {
                    file << "#content=" << c << '\n';
                }
                content_written = true;
            }
            // Skip the original line.
        } else {
            // This is not a data or content line, so write it out as-is.
            file << line << '\n';
        }
    }
    
    return true;
}
