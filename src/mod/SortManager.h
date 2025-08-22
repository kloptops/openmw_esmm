#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "ModManager.h"

namespace fs = boost::filesystem;

struct SortRule {
    std::string pattern;
    int priority;
    int line_number; // To maintain stable sort order for rules with the same priority
};

class SortManager {
public:
    // Loads and parses the INI file
    bool load_rules(const fs::path& ini_path);

    // Sorts the given list of data paths based on the loaded rules
    void sort_data_paths(std::vector<fs::path>& data_paths, const std::vector<fs::path>& mod_source_dirs);

    // Sorts the given list of content files based on the loaded rules
    void sort_content_files(std::vector<ContentFile>& content_files);

    // Are any rules loaded?
    bool rules_loaded() { return (!data_rules.empty() || !content_rules.empty()); }

private:
    std::vector<SortRule> data_rules;
    std::vector<SortRule> content_rules;
};
