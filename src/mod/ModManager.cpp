#include "ModManager.h"
#include <iostream>
#include <algorithm>
#include <regex>
#include <set>

// Uncomment this line to get verbose output from the parser to the console
#define DEBUG_PARSER

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
static std::vector<std::string> find_plugins_in_path(const fs::path& path) {
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

// --- scan_mods is unchanged ---
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

// --- sync_state_from_config is unchanged ---
void ModManager::sync_state_from_config(const std::vector<std::string>& active_data, const std::vector<std::string>& active_content) {
    std::set<std::string> data_set(active_data.begin(), active_data.end());
    std::set<std::string> content_set(active_content.begin(), active_content.end());

    for (auto& mod : mod_definitions) {
        bool is_mod_active = false;
        for (auto& group : mod.option_groups) {
            for (auto& option : group.options) {
                if (data_set.count(option.path.string())) {
                    option.enabled = true;
                    is_mod_active = true;
                }
            }
        }
        for (auto& plugin : mod.standalone_plugins) {
             if (!plugin.discovered_plugins.empty() && content_set.count(plugin.discovered_plugins[0])) {
                plugin.enabled = true;
                is_mod_active = true;
             }
        }
        mod.enabled = is_mod_active;
    }
}

void ModManager::update_active_lists() {
    // --- PART 1: DATA PATHS ---

    // 1. Get a set of all data paths that SHOULD be active from the mod configuration
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

    // 2. Build a new list, preserving the order of currently active items
    std::vector<fs::path> new_active_data_paths;
    for (const auto& path : active_data_paths) {
        if (enabled_data_paths.count(path)) {
            new_active_data_paths.push_back(path);
        }
    }

    // 3. Add any newly enabled paths to the end of the list
    for (const auto& mod : mod_definitions) {
        if (!mod.enabled) continue;
        for (const auto& group : mod.option_groups) {
            for (const auto& option : group.options) {
                if (option.enabled) {
                    bool found = false;
                    for(const auto& p : new_active_data_paths) if(p == option.path) found = true;
                    if (!found) {
                        new_active_data_paths.push_back(option.path);
                    }
                }
            }
        }
    }
    
    // 4. Replace the old list with the new, order-preserved list
    active_data_paths = new_active_data_paths;


    // --- PART 2: CONTENT FILES (same logic) ---

    // 1. Get a map of all plugins that SHOULD be active and their source mods
    std::map<std::string, std::string> enabled_plugins; // map<plugin_name, source_mod>
    for (const auto& mod : mod_definitions) {
        if (!mod.enabled) continue;
        for (const auto& group : mod.option_groups) {
            for (const auto& option : group.options) {
                if (option.enabled) {
                    for (const auto& plugin_name : option.discovered_plugins) {
                        enabled_plugins[plugin_name] = mod.name;
                    }
                }
            }
        }
    }

    // 2. Build a new list, preserving order and enabled state of existing plugins
    std::vector<ContentFile> new_active_content_files;
    std::set<std::string> plugins_in_new_list;
    for (const auto& content_file : active_content_files) {
        if (enabled_plugins.count(content_file.name)) {
            new_active_content_files.push_back(content_file); // Keep existing object to preserve its enabled state
            plugins_in_new_list.insert(content_file.name);
        }
    }

    // 3. Add any newly enabled plugins to the end of the list
    for (const auto& mod : mod_definitions) {
        if (!mod.enabled) continue;
        for (const auto& group : mod.option_groups) {
            for (const auto& option : group.options) {
                if (option.enabled) {
                    for (const auto& plugin_name : option.discovered_plugins) {
                        if (plugins_in_new_list.find(plugin_name) == plugins_in_new_list.end()) {
                            new_active_content_files.push_back({plugin_name, true, mod.name});
                            plugins_in_new_list.insert(plugin_name);
                        }
                    }
                }
            }
        }
    }
    
    // 4. Replace the old list
    active_content_files = new_active_content_files;
}

