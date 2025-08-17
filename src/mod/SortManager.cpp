#include "SortManager.h"
#include <fstream>
#include <iostream> // Needed for std::cout
#include <algorithm>
#include <cctype>

// --- THE DEBUG SWITCH ---
// To see the detailed sorting log, leave this defined.
// To turn off the logging for a final build, comment this line out.
#define DEBUG_SORTER

// Simple wildcard match (fnmatch style)
// ? matches any single character
// * matches any sequence of characters (including an empty sequence)
static bool wildcard_match(const std::string& text, const std::string& pattern) {
    auto text_it = text.begin();
    auto patt_it = pattern.begin();
    auto text_backtrack = text.end();
    auto patt_backtrack = pattern.end();

    while (text_it != text.end()) {
        if (patt_it != pattern.end() && *patt_it == '*') {
            patt_backtrack = patt_it++;
            text_backtrack = text_it;
        } else if (patt_it != pattern.end() && (*patt_it == '?' || ::tolower(*patt_it) == ::tolower(*text_it))) {
            patt_it++;
            text_it++;
        } else if (patt_backtrack != pattern.end()) {
            patt_it = patt_backtrack + 1;
            text_it = ++text_backtrack;
        } else {
            return false;
        }
    }

    while (patt_it != pattern.end() && *patt_it == '*') {
        patt_it++;
    }

    return patt_it == pattern.end();
}

bool SortManager::load_rules(const fs::path& ini_path) {
    data_rules.clear();
    content_rules.clear();

    std::ifstream file(ini_path.string());
    if (!file.is_open()) {
        // It's not an error if the file doesn't exist
        return false;
    }

    enum class Section { NONE, DATA, CONTENT };
    Section current_section = Section::NONE;
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        line_num++;
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (line == "[data]") { current_section = Section::DATA; continue; }
        if (line == "[content]") { current_section = Section::CONTENT; continue; }

        if (current_section != Section::NONE) {
            std::string pattern;
            int priority = 9999; // Default high priority for rules without one
            size_t eq_pos = line.find('=');

            if (eq_pos != std::string::npos) {
                pattern = line.substr(0, eq_pos);
                try {
                    priority = std::stoi(line.substr(eq_pos + 1));
                } catch(...) {}
            } else {
                pattern = line;
            }

            if (!pattern.empty()) {
                if (current_section == Section::DATA) {
                    data_rules.push_back({pattern, priority, line_num});
                } else {
                    content_rules.push_back({pattern, priority, line_num});
                }
            }
        }
    }
    return true;
}

// --- UPDATED HELPER with DEBUG LOGGING ---
static int get_priority(const std::vector<SortRule>& rules, const std::string& name) {
    #ifdef DEBUG_SORTER
    std::cout << "  Item: '" << name << "'\n";
    #endif

    for (const auto& rule : rules) {
        if (wildcard_match(name, rule.pattern)) {
            #ifdef DEBUG_SORTER
            std::cout << "    -> Matched rule: \"" << rule.pattern << "\" -> Priority: " << rule.priority << std::endl;
            #endif
            return rule.priority;
        }
    }

    #ifdef DEBUG_SORTER
    std::cout << "    -> No specific rule matched. Using default priority 9999." << std::endl;
    #endif
    return 9999; // Default priority if no match
}


// --- UPDATED SORTING FUNCTIONS with DEBUG LOGGING ---
void SortManager::sort_data_paths(std::vector<fs::path>& data_paths, const fs::path& mod_data_dir) {
    #ifdef DEBUG_SORTER
    std::cout << "\n--- DEBUG: Auto-Sorting Data Paths ---\n";
    #endif

    std::stable_sort(data_paths.begin(), data_paths.end(),
        [&](const fs::path& a, const fs::path& b) {
            std::string a_rel = a.lexically_relative(mod_data_dir).string();
            std::string b_rel = b.lexically_relative(mod_data_dir).string();
            
            int a_priority = get_priority(data_rules, a_rel);
            int b_priority = get_priority(data_rules, b_rel);

            if (a_priority != b_priority) {
                return a_priority < b_priority;
            }
            
            std::string a_lower = a_rel;
            std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), ::tolower);
            std::string b_lower = b_rel;
            std::transform(b_lower.begin(), b_lower.end(), b_lower.begin(), ::tolower);
            return a_lower < b_lower;
        });
    
    #ifdef DEBUG_SORTER
    std::cout << "--- Sort Complete. Final Order: ---\n";
    for(const auto& path : data_paths) {
        std::cout << "  " << path.lexically_relative(mod_data_dir).string() << std::endl;
    }
    std::cout << "-------------------------------------\n\n";
    #endif
}

void SortManager::sort_content_files(std::vector<ContentFile>& content_files) {
    #ifdef DEBUG_SORTER
    std::cout << "\n--- DEBUG: Auto-Sorting Content Files ---\n";
    #endif

    std::stable_sort(content_files.begin(), content_files.end(),
        [&](const ContentFile& a, const ContentFile& b) {
            int a_priority = get_priority(content_rules, a.name);
            int b_priority = get_priority(content_rules, b.name);

            if (a_priority != b_priority) {
                return a_priority < b_priority;
            }

            std::string a_lower = a.name;
            std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), ::tolower);
            std::string b_lower = b.name;
            std::transform(b_lower.begin(), b_lower.end(), b_lower.begin(), ::tolower);
            return a_lower < b_lower;
        });

    #ifdef DEBUG_SORTER
    std::cout << "--- Sort Complete. Final Order: ---\n";
    for(const auto& file : content_files) {
        std::cout << "  " << file.name << std::endl;
    }
    std::cout << "-------------------------------------\n\n";
    #endif
}
