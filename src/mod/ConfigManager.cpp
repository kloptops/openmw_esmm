#include "ConfigManager.h"
#include <fstream>
#include <iostream>

bool ConfigManager::load(const fs::path& config_path)
{
    std::ifstream file(config_path.string());

    if (!file.is_open())
    {
        std::cerr << "Warning: Could not open " << config_path << ". A new one may be created." << std::endl;
        return false;
    }

    lines_before_data.clear();
    lines_between.clear();
    lines_after_content.clear();
    data_paths.clear();
    content_files.clear();

    enum class ParseState { BeforeData,InData, Between, InContent, AfterContent };
    ParseState state = ParseState::BeforeData;

    std::string line;
    while (std::getline(file, line))
    {
        // Trim whitespace if you want, but for now we keep it simple.
        if (line.find("--BEGIN DATA--") != std::string::npos)
        {
            state = ParseState::InData;
            continue;
        }
        else if (line.find("--END DATA--") != std::string::npos)
        {
            state = ParseState::Between;
            continue;
        }
        else if (line.find("--BEGIN CONTENT--") != std::string::npos)
        {
            state = ParseState::InContent;
            continue;
        }
        else if (line.find("--END CONTENT--") != std::string::npos)
        {
            state = ParseState::AfterContent;
            continue;
        }

        switch (state)
        {
        case ParseState::BeforeData:    lines_before_data.push_back(line); break;
        case ParseState::Between:       lines_between.push_back(line); break;
        case ParseState::AfterContent:  lines_after_content.push_back(line); break;
        case ParseState::InData:
            if (line.rfind("data=", 0) == 0) data_paths.push_back(line.substr(5));
            break;
        case ParseState::InContent:
            if (line.rfind("content=", 0) == 0) content_files.push_back(line.substr(8));
            break;
        }
    }
    return true;
}

bool ConfigManager::save(const fs::path& config_path)
{
    std::ofstream file(config_path.string());

    if (!file.is_open())
    {
        std::cerr << "ERROR: Could not open " << config_path << " for writing." << std::endl;
        return false;
    }

    auto write_lines = [&](const std::vector<std::string>& lines)
    {
        for (const auto& l : lines) file << l << '\n';
    };

    write_lines(lines_before_data);
    file << "## --BEGIN DATA--\n";
    for (const auto& p : data_paths) file << "data=" << p << '\n';
    file << "## --END DATA--\n";
    write_lines(lines_between);
    file << "## --BEGIN CONTENT--\n";
    for (const auto& c : content_files) file << "content=" << c << '\n';
    file << "## --END CONTENT--\n";
    write_lines(lines_after_content);
    
    return true;
}
