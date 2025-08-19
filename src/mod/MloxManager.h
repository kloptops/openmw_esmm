#pragma once
#include "ModManager.h" // For ContentFile struct
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// In src/mod/MloxManager.h
struct PriorityRule {
    std::string plugin_name;
    int line_number;
};

class MloxManager {
public:
    // Loads and parses the mlox_base.txt rule file
    bool load_rules(const std::vector<fs::path>& mlox_rules_paths);

    // The core function: sorts the given list of content files
    void sort_content_files(std::vector<ContentFile>& content_files, const std::vector<fs::path>& base_data_paths, const std::vector<fs::path>& mod_data_paths);

    // Retrieves any notes or warnings for a specific plugin
    std::vector<std::string> get_messages_for_plugin(const std::string& plugin_name) const;

private:
    // A map where: key is a plugin, value is a list of plugins that must load BEFORE it.
    std::map<std::string, std::vector<std::string>> dependencies;
    
    // A map to store notes, warnings, etc.
    std::map<std::string, std::vector<std::string>> messages;

    // Special priority hints
    std::vector<PriorityRule> near_start_plugins;
    std::vector<PriorityRule> near_end_plugins;
};
