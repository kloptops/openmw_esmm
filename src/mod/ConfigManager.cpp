#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <string>

// Helper to trim leading/trailing whitespace
static std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) {
        return s;
    }
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

bool ConfigManager::load(const fs::path& config_path) {
    // Clear all previous data
    lines_before_base_data.clear();
    lines_between_data.clear();
    lines_between_content.clear();
    lines_after_mod_content.clear();
    base_data.clear();
    mod_data.clear();
    base_content.clear();
    mod_content.clear();
    disabled_content.clear();

    std::ifstream file(config_path.string());
    if (!file.is_open()) return false;

    enum class Block { PRE, BASE_DATA, MOD_DATA, BETWEEN_DATA, BASE_CONTENT, MOD_CONTENT, BETWEEN_CONTENT, POST };
    Block current_block = Block::PRE;

    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed_line = trim(line);
        
        // State transitions
        if (trimmed_line.find("--BEGIN BASE DATA--") != std::string::npos) { current_block = Block::BASE_DATA; continue; }
        if (trimmed_line.find("--END BASE DATA--") != std::string::npos) { current_block = Block::BETWEEN_DATA; continue; }
        if (trimmed_line.find("--BEGIN MOD DATA--") != std::string::npos) { current_block = Block::MOD_DATA; continue; }
        if (trimmed_line.find("--END MOD DATA--") != std::string::npos) { current_block = Block::BETWEEN_CONTENT; continue; }
        if (trimmed_line.find("--BEGIN BASE CONTENT--") != std::string::npos) { current_block = Block::BASE_CONTENT; continue; }
        if (trimmed_line.find("--END BASE CONTENT--") != std::string::npos) { current_block = Block::BETWEEN_CONTENT; continue; }
        if (trimmed_line.find("--BEGIN MOD CONTENT--") != std::string::npos) { current_block = Block::MOD_CONTENT; continue; }
        if (trimmed_line.find("--END MOD CONTENT--") != std::string::npos) { current_block = Block::POST; continue; }
        
        // Data parsing
        if (line.rfind("data=", 0) == 0) {
            std::string path = line.substr(5);
            if (current_block == Block::BASE_DATA) base_data.push_back(path);
            else if (current_block == Block::MOD_DATA) mod_data.push_back(path);
        } else if (line.rfind("content=", 0) == 0) {
            std::string plugin = line.substr(8);
            if (current_block == Block::BASE_CONTENT) base_content.push_back(plugin);
            else if (current_block == Block::MOD_CONTENT) mod_content.push_back(plugin);
        } else if (line.rfind("#content=", 0) == 0) {
            disabled_content.push_back(line.substr(9));
        } else {
            switch(current_block) {
                case Block::PRE:              lines_before_base_data.push_back(line); break;
                case Block::BETWEEN_DATA:     lines_between_data.push_back(line); break;
                case Block::BETWEEN_CONTENT:  lines_between_content.push_back(line); break;
                case Block::POST:             lines_after_mod_content.push_back(line); break;
                default: break;
            }
        }
    }
    return true;
}

bool ConfigManager::save(const fs::path& config_path) {
    std::ofstream file(config_path.string());
    if (!file.is_open()) return false;

    auto write_lines = [&](const std::vector<std::string>& lines) {
        for (const auto& l : lines) file << l << '\n';
    };

    write_lines(lines_before_base_data);
    file << "##--BEGIN BASE DATA--\n";
    for (const auto& p : base_data) file << "data=" << p << '\n';
    file << "##--END BASE DATA--\n";
    write_lines(lines_between_data);
    file << "##--BEGIN MOD DATA--\n";
    for (const auto& p : mod_data) file << "data=" << p << '\n';
    file << "##--END MOD DATA--\n";
    write_lines(lines_between_content);
    file << "##--BEGIN BASE CONTENT--\n";
    for (const auto& c : base_content) file << "content=" << c << '\n';
    file << "##--END BASE CONTENT--\n";
    file << "##--BEGIN MOD CONTENT--\n";
    for (const auto& c : mod_content) file << "content=" << c << '\n';
    for (const auto& c : disabled_content) file << "#content=" << c << '\n';
    file << "##--END MOD CONTENT--\n";
    write_lines(lines_after_mod_content);
    
    return true;
}
