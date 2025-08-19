#include "MloxManager.h"
#include "PluginParser.h"
#include "../utils/Logger.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <deque>
#include <set>
#include <sstream>

static std::string normalize_name(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lower;
}

// --- THE FINAL, CORRECTED, ROBUST PARSER ---
bool MloxManager::load_rules(const std::vector<fs::path>& mlox_rules_paths) {
    dependencies.clear(); messages.clear(); near_start_plugins.clear(); near_end_plugins.clear();
    bool file_opened_successfully = false;
    int line_num = 0;

    for (const auto& path : mlox_rules_paths) {
        std::ifstream file(path.string());
        if (!file.is_open()) continue;
        
        file_opened_successfully = true;
        LOG_INFO("Loading mlox rules from ", path.string());
        std::string line, last_rule_type, last_plugin_for_order;
        
        while (std::getline(file, line)) {
            line_num++;
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty() || line[0] == ';') {
                if (line.empty()) last_plugin_for_order = "";
                continue;
            }

            if (line.front() == '[' && line.back() == ']') {
                last_plugin_for_order = "";
                std::string rule_content = line.substr(1, line.length() - 2);
                std::string rule_type, rule_data;
                size_t space_pos = rule_content.find(' ');

                if (space_pos != std::string::npos) {
                    rule_type = rule_content.substr(0, space_pos);
                    rule_data = rule_content.substr(space_pos + 1);
                } else {
                    rule_type = rule_content;
                }
                
                std::transform(rule_type.begin(), rule_type.end(), rule_type.begin(), ::toupper);
                last_rule_type = rule_type;
                LOG_DEBUG("L", line_num, ": Detected Rule '", last_rule_type, "' with data '", rule_data, "'");

                if (!rule_data.empty()) {
                    if (last_rule_type == "ORDER") {
                        std::istringstream iss(rule_data);
                        std::string plugin_a, plugin_b;
                        if (iss >> plugin_a && iss >> plugin_b) {
                            dependencies[normalize_name(plugin_b)].push_back(normalize_name(plugin_a));
                            LOG_DEBUG("  -> Parsed single-line ORDER: '", plugin_b, "' AFTER '", plugin_a, "'");
                        }
                    } else if (last_rule_type == "NEARSTART") {
                        near_start_plugins.push_back({normalize_name(rule_data), line_num});
                    } else if (last_rule_type == "NEAREND") {
                        near_end_plugins.push_back({normalize_name(rule_data), line_num});
                    }
                }
            } else {
                if (last_rule_type == "ORDER") {
                    if (last_plugin_for_order.empty()) {
                        last_plugin_for_order = normalize_name(line);
                    } else {
                        std::string current_plugin = normalize_name(line);
                        dependencies[current_plugin].push_back(last_plugin_for_order);
                        last_plugin_for_order = current_plugin;
                    }
                } else if (last_rule_type == "NEARSTART") {
                    near_start_plugins.push_back({normalize_name(line), line_num});
                } else if (last_rule_type == "NEAREND") {
                    near_end_plugins.push_back({normalize_name(line), line_num});
                }
            }
        }
    }
    LOG_INFO("Successfully loaded ", dependencies.size(), " mlox ordering rules.");
    return file_opened_successfully;
}


// Searches a list of directories in reverse to find a plugin, mimicking OpenMW's VFS.
static fs::path find_plugin_path(const std::string& plugin_name, const std::vector<fs::path>& search_paths) {
    // Iterate backwards (rbegin to rend)
    for (auto it = search_paths.rbegin(); it != search_paths.rend(); ++it) {
        fs::path potential_path = *it / plugin_name;
        if (fs::exists(potential_path)) {
            return potential_path;
        }
    }
    return {}; // Return empty path if not found
}


void MloxManager::sort_content_files(std::vector<ContentFile>& content_files, const std::vector<fs::path>& base_data_paths, const std::vector<fs::path>& mod_data_paths) {
    if (dependencies.empty() && near_start_plugins.empty() && near_end_plugins.empty()) {
        LOG_WARN("No mlox rules loaded, skipping sort.");
        return;
    }

    LOG_INFO("Starting mlox auto-sort process...");

    LOG_INFO("--- Sort Complete. Starting Order: ---");
    for(const auto& cf : content_files) { LOG_INFO("  ", cf.name); }
    LOG_INFO("-------------------------------------");

    // --- Build the full dependency graph from headers and mlox rules ---
    std::map<std::string, std::vector<std::string>> dependencies_map;
    std::map<std::string, const ContentFile*> content_map;
    for (const auto& cf : content_files) content_map[normalize_name(cf.name)] = &cf;

    // 1a. Populate graph from plugin headers (hard dependencies)
    LOG_DEBUG("Step 1a: Building Dependency Graph from Plugin Headers...");
    PluginParser parser;
    std::vector<fs::path> all_search_paths = base_data_paths;
    all_search_paths.insert(all_search_paths.end(), mod_data_paths.begin(), mod_data_paths.end());
    for (const auto& cf : content_files) {
        fs::path plugin_path = find_plugin_path(cf.name, all_search_paths);
        if (!plugin_path.empty()) {
            std::vector<std::string> masters = parser.get_masters(plugin_path);
            for (const auto& master : masters) {
                if (content_map.count(normalize_name(master))) {
                    dependencies_map[normalize_name(cf.name)].push_back(normalize_name(master));
                }
            }
        }
    }
    
    // 1b. Populate graph from mlox files (soft dependencies)
    LOG_DEBUG("Step 1b: Building Dependency Graph from mlox Rules...");
    for (const auto& pair : dependencies) {
        if (content_map.count(pair.first)) {
            for (const auto& dep : pair.second) {
                if (content_map.count(dep)) {
                    dependencies_map[pair.first].push_back(dep);
                }
            }
        }
    }

    // --- STAGE 2: The Iterative Sorter ---
    LOG_DEBUG("Step 2: Iteratively sorting based on dependencies...");
    for (int i = 0; i < 100; ++i) { // Max 100 iterations to prevent infinite loop on cycles
        bool swapped_this_pass = false;
        for (size_t j = 0; j < content_files.size(); ++j) {
            for (size_t k = j + 1; k < content_files.size(); ++k) {
                std::string norm_j = normalize_name(content_files[j].name);
                std::string norm_k = normalize_name(content_files[k].name);
                auto it = dependencies_map.find(norm_j);
                if (it != dependencies_map.end()) {
                    for (const auto& dep : it->second) {
                        if (dep == norm_k) {
                            LOG_DEBUG("  - Swapping '", content_files[j].name, "' and '", content_files[k].name, "' to satisfy dependency.");
                            std::swap(content_files[j], content_files[k]);
                            swapped_this_pass = true;
                            break;
                        }
                    }
                }
                if (swapped_this_pass) break;
            }
            if (swapped_this_pass) break;
        }
        if (!swapped_this_pass) {
            LOG_DEBUG("  -> List is stable after ", i + 1, " passes.");
            break;
        }
    }

    // --- STAGE 3: Apply NearStart and NearEnd as forceful pre-sort steps ---
    // This happens *after* the dependency sort is stable.
    LOG_DEBUG("Step 3: Applying NearEnd rules...");
    for (auto it = near_end_plugins.rbegin(); it != near_end_plugins.rend(); ++it) {
        auto& rule = *it;
        for (size_t i = 0; i < content_files.size(); ++i) {
            if (normalize_name(content_files[i].name) == rule.plugin_name) {
                ContentFile temp = content_files[i];
                content_files.erase(content_files.begin() + i);
                content_files.push_back(temp);
                break;
            }
        }
    }

    LOG_DEBUG("Step 4: Applying NearStart rules...");
    for (auto it = near_start_plugins.rbegin(); it != near_start_plugins.rend(); ++it) {
        auto& rule = *it;
        for (size_t i = 0; i < content_files.size(); ++i) {
            if (normalize_name(content_files[i].name) == rule.plugin_name) {
                ContentFile temp = content_files[i];
                content_files.erase(content_files.begin() + i);
                content_files.insert(content_files.begin(), temp);
                break;
            }
        }
    }

    // --- STAGE 5: The plox ESM Heuristic Pass ---
    LOG_DEBUG("Step 5: Applying ESM heuristic...");
    std::vector<ContentFile> esms;
    std::vector<ContentFile> others;
    for (const auto& cf : content_files) {
        std::string norm_name = normalize_name(cf.name);
        if (norm_name.rfind(".esm") == norm_name.length() - 4) esms.push_back(cf);
        else others.push_back(cf);
    }
    
    content_files.clear();
    content_files.insert(content_files.end(), esms.begin(), esms.end());
    content_files.insert(content_files.end(), others.begin(), others.end());

    // As a final guarantee, force the three official masters to the absolute top in order.
    auto force_to_top = [&](const std::string& name) {
        auto it = std::find_if(content_files.begin(), content_files.end(), [&](const ContentFile& cf){
            return normalize_name(cf.name) == name;
        });
        if (it != content_files.end()) {
            ContentFile master = *it;
            content_files.erase(it);
            content_files.insert(content_files.begin(), master);
        }
    };

    force_to_top("bloodmoon.esm");
    force_to_top("tribunal.esm");
    force_to_top("morrowind.esm");

    LOG_INFO("Mlox auto-sort complete.");

    LOG_INFO("--- Sort Complete. Final Order: ---");
    for(const auto& cf : content_files) { LOG_INFO("  ", cf.name); }
    LOG_INFO("-------------------------------------");
}


std::vector<std::string> MloxManager::get_messages_for_plugin(const std::string& plugin_name) const {
    auto it = messages.find(normalize_name(plugin_name));
    if (it != messages.end()) {
        return it->second;
    }
    return {};
}