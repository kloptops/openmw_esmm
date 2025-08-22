#include "SortManager.h"
#include <fstream>
#include <iostream> // Needed for std::cout
#include <algorithm>
#include <cctype>

// --- THE DEBUG SWITCH ---
// To see the detailed sorting log, leave this defined.
// To turn off the logging for a final build, comment this line out.
#define DEBUG_SORTER

#define MAX_PRIORITY 99999

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

// Finds the correct relative path for sorting rules based on a list of possible root directories.
static std::string get_sortable_string(const fs::path& path, const std::vector<fs::path>& mod_source_dirs) {
    // The base game data path is a special case; its rule is just "Data Files" or "Data Files/"
    if (fs::exists(path / "Morrowind.esm")) {
        return "Data Files/";
    }

    for (const auto& source_dir : mod_source_dirs) {
        if (!source_dir.empty() && path.string().find(source_dir.string()) == 0) {
            return path.lexically_relative(source_dir).string();
        }
    }
    // Fallback for paths not found in a source dir (should be rare)
    return path.filename().string();
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
            int priority = MAX_PRIORITY; // Default high priority for rules without one
            size_t eq_pos = line.find('=');

            if (eq_pos != std::string::npos) {
                pattern = line.substr(0, eq_pos);
                try {
                    priority = std::stoi(line.substr(eq_pos + 1));
                } catch(...) {}
            } else {
                pattern = line;
            }

            // Clamp priority between 0 and MAX_PRIORITY
            priority = ((priority < 0) ? 0 : ((priority <= MAX_PRIORITY) ? priority : MAX_PRIORITY));

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

    return MAX_PRIORITY; // Default priority if no match
}


void SortManager::sort_data_paths(std::vector<fs::path>& data_paths, const std::vector<fs::path>& mod_source_dirs) {
    #ifdef DEBUG_SORTER
    std::cout << "\n--- DEBUG: Auto-Sorting Data Paths ---\n";
    #endif

    // --- Find and preserve the base game data path ---
    fs::path game_data_path;
    auto it = std::find_if(data_paths.begin(), data_paths.end(), [](const fs::path& p) {
        return fs::exists(p / "Morrowind.esm");
    });
    if (it != data_paths.end()) {
        game_data_path = *it;
        data_paths.erase(it);
        #ifdef DEBUG_SORTER
        std::cout << "  Found game data path: " << game_data_path.string() << ". Setting as highest priority.\n";
        #endif
    }

    std::stable_sort(data_paths.begin(), data_paths.end(),
        [&](const fs::path& a, const fs::path& b) {
            std::string a_rel = get_sortable_string(a, mod_source_dirs);
            std::string b_rel = get_sortable_string(b, mod_source_dirs);
            
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
    
    // --- Insert the game data path back at the beginning ---
    if (!game_data_path.empty()) {
        data_paths.insert(data_paths.begin(), game_data_path);
    }
    
    #ifdef DEBUG_SORTER
    std::cout << "--- Sort Complete. Final Order: ---\n";
    for(const auto& path : data_paths) {
        std::cout << "  " << get_sortable_string(path, mod_source_dirs) << std::endl;
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
