#include "ModManager.h"
#include <iostream>
#include <algorithm>
#include <regex>
#include <set>

// Uncomment this line to get verbose output from the parser to the console
// #define DEBUG_PARSER

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

bool is_plugin_file(const fs::path& path) {
    if (!fs::is_regular_file(path)) return false;
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".esp" || ext == ".esm" || ext == ".omwscripts" || ext == ".omwaddon";
}

// Now correctly handles the "Data Files" case without flagging the parent.
static bool is_data_directory(const fs::path& dir_path) {
    if (!fs::is_directory(dir_path)) return false;
    try {
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            if (fs::is_directory(entry)) {
                std::string dirname = entry.path().filename().string();
                std::transform(dirname.begin(), dirname.end(), dirname.begin(), ::tolower);
                // A directory is a data directory if it CONTAINS these folders.
                if (dirname == "meshes" || dirname == "textures") return true;
            } else if (is_plugin_file(entry.path())) {
                return true;
            }
        }
    } catch (const fs::filesystem_error&) {}
    return false;
}

// Helper to find all plugins within a given path (non-recursive)
std::vector<std::string> find_plugins_in_path(const fs::path& path) {
    std::vector<std::string> plugins;
    if (!fs::is_directory(path)) return plugins;
    try {
        for(const auto& p_entry : fs::directory_iterator(path)) {
            if(is_plugin_file(p_entry.path())) {
                plugins.push_back(p_entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error&) {}
    return plugins;
}

// New recursive helper to find optional data paths
static void find_optional_data_paths_recursive(const fs::path& current_path, std::vector<fs::path>& found_paths) {
    if (is_data_directory(current_path)) {
        found_paths.push_back(current_path);
        return; // Found a valid data path, stop recursing down this branch.
    }
    try {
        for (const auto& entry : fs::directory_iterator(current_path)) {
            if (fs::is_directory(entry.path())) {
                find_optional_data_paths_recursive(entry.path(), found_paths);
            }
        }
    } catch (const fs::filesystem_error&) {}
}

// --- THE FINAL, RULE-BASED PARSER ---
ModDefinition parse_mod_directory(const fs::path& mod_root_path) {
    ModDefinition mod;
    mod.name = mod_root_path.filename().string();
    mod.root_path = mod_root_path;
    std::set<fs::path> processed_paths;

    // --- RULE 1: Numbered Groups ---
    std::map<int, ModOptionGroup> numbered_groups;
    for (const auto& entry : fs::directory_iterator(mod_root_path)) {
        if (!fs::is_directory(entry.path())) continue;
        std::string dirname = entry.path().filename().string();
        std::regex num_pattern("^(\\d+)\\s*(.*)");
        std::smatch match;
        if (std::regex_match(dirname, match, num_pattern)) {
            int group_num = std::stoi(match[1].str());
            std::string name_part = match[2].str().empty() ? dirname : match[2].str();
            if (numbered_groups.find(group_num) == numbered_groups.end()) {
                numbered_groups[group_num].name = "Group " + std::to_string(group_num);
                numbered_groups[group_num].type = ModOptionGroup::SINGLE_CHOICE;
                numbered_groups[group_num].required = (group_num == 0);
            }
            numbered_groups[group_num].options.emplace_back(ModOption{name_part, entry.path(), find_plugins_in_path(entry.path())});
            processed_paths.insert(entry.path());
        }
    }
    for (const auto& pair : numbered_groups) mod.option_groups.push_back(pair.second);

    // --- RULE 2 & 3: Main `Data Files` or Root as Main ---
    fs::path main_data_path;
    fs::path root_data_files = mod_root_path / "Data Files";
    if (fs::exists(root_data_files) && is_data_directory(root_data_files)) {
        main_data_path = root_data_files;
    } else if (is_data_directory(mod_root_path)) {
        main_data_path = mod_root_path;
    }

    if (!main_data_path.empty()) {
        ModOptionGroup main_group;
        main_group.name = "Main";
        main_group.type = ModOptionGroup::MULTIPLE_CHOICE;
        main_group.required = true;
        main_group.options.emplace_back(ModOption{main_data_path.filename().string(), main_data_path, find_plugins_in_path(main_data_path), true});
        mod.option_groups.insert(mod.option_groups.begin(), main_group); // Add to the front
        processed_paths.insert(main_data_path);
        processed_paths.insert(mod_root_path); // Mark root as processed if it was the main path
    }

    // --- RULE 4: Recursive Search for Optionals ---
    std::vector<fs::path> optional_paths;
    for (const auto& entry : fs::directory_iterator(mod_root_path)) {
        if (fs::is_directory(entry.path()) && processed_paths.find(entry.path()) == processed_paths.end()) {
            find_optional_data_paths_recursive(entry.path(), optional_paths);
        }
    }
    
    if (!optional_paths.empty()) {
        ModOptionGroup optional_group;
        optional_group.name = "Optional";
        optional_group.type = ModOptionGroup::MULTIPLE_CHOICE;
        for (const auto& path : optional_paths) {
            std::string name = path.lexically_relative(mod_root_path).string();
            optional_group.options.emplace_back(ModOption{name, path, find_plugins_in_path(path)});
        }
        mod.option_groups.push_back(optional_group);
    }
    
    // Set all parent pointers after the groups vector is finalized.
    for (auto& group : mod.option_groups) {
        for (auto& option : group.options) {
            option.parent_group = &group;
        }
    }
    return mod;
}

// =============================================================================
// MOD MANAGER CLASS IMPLEMENTATION
// =============================================================================

void ModManager::scan_mods(const fs::path& mod_data_path) {
    mod_definitions.clear();
    if (!fs::exists(mod_data_path) || !fs::is_directory(mod_data_path)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(mod_data_path)) {
        if (fs::is_directory(entry.path())) {
            try {
                mod_definitions.push_back(parse_mod_directory(entry.path()));
            } catch (const std::exception& e) {
                std::cerr << "Error parsing mod " << entry.path() << ": " << e.what() << std::endl;
            }
        }
    }
}

void ModManager::sync_ui_state_from_active_lists() {
    std::set<std::string> data_set;
    for(const auto& p : active_data_paths) data_set.insert(p.string());

    for (auto& mod : mod_definitions) {
        bool is_mod_active = false;
        for (auto& group : mod.option_groups) {
            for (auto& option : group.options) {
                // Check if this option's path is in the active set
                if (data_set.count(option.path.string())) {
                    option.enabled = true;
                    is_mod_active = true;
                } else {
                    option.enabled = false;
                }
            }
        }
        mod.enabled = is_mod_active;
    }
}

void ModManager::update_active_lists() {
    // --- PART 1: DATA PATHS ---
    // This is the same as before: update active_data_paths based on UI checkboxes.
    std::set<fs::path> enabled_data_paths;
    for (const auto& mod : mod_definitions) {
        if (!mod.enabled) continue;
        for (const auto& group : mod.option_groups) {
            for (const auto& option : group.options) {
                if (option.enabled) {
                    enabled_data_paths.insert(option.path);
                }
            }
        }
    }

    std::vector<fs::path> new_active_data_paths;
    std::set<fs::path> paths_in_new_list;
    for (const auto& path : active_data_paths) {
        if (enabled_data_paths.count(path)) {
            new_active_data_paths.push_back(path);
            paths_in_new_list.insert(path);
        }
    }
    for (const auto& path : enabled_data_paths) {
        if (paths_in_new_list.find(path) == paths_in_new_list.end()) {
            new_active_data_paths.push_back(path);
        }
    }
    active_data_paths = new_active_data_paths;

    // --- PART 2: CONTENT FILES (NEW, SIMPLER LOGIC) ---
    
    // A. Discover all plugins available in the *new* set of active data paths.
    std::map<std::string, std::string> available_plugins; // map<plugin_name, source_mod>
    for (const auto& p : active_data_paths) {
        // Find which mod definition this data path belongs to
        std::string source_mod_name = "Unknown";
        bool found = false;
        for (const auto& mod : mod_definitions) {
            for (const auto& group : mod.option_groups) {
                for (const auto& option : group.options) {
                    if (option.path == p) {
                        source_mod_name = mod.name;
                        found = true; break;
                    }
                }
                if (found) break;
            }
            if (found) break;
        }
        for (const auto& plugin_name : find_plugins_in_path(p)) {
            available_plugins[plugin_name] = source_mod_name;
        }
    }

    // B. Rebuild the active_content_files list.
    std::vector<ContentFile> new_active_content_files;
    std::set<std::string> plugins_in_new_list;

    // First pass: Preserve the order and state of existing files that are still available.
    for (const auto& existing_cf : active_content_files) {
        if (available_plugins.count(existing_cf.name)) {
            new_active_content_files.push_back(existing_cf);
            plugins_in_new_list.insert(existing_cf.name);
        }
    }

    // Second pass: Add any brand new, discovered plugins to the end as disabled.
    for (const auto& pair : available_plugins) {
        if (plugins_in_new_list.find(pair.first) == plugins_in_new_list.end()) {
            ContentFile new_file;
            new_file.name = pair.first;
            new_file.enabled = false;   // Set to DISABLED by default.
            new_file.source_mod = pair.second;
            new_file.is_new = true;     // Flag it as NEW for the UI.
            new_active_content_files.push_back(new_file);
        }
    }
    
    active_content_files = new_active_content_files;
}
