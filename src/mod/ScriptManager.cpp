#include "ScriptManager.h"
#include "../utils/Logger.h"
#include <fstream>
#include <algorithm>
#include <cctype>

// Helper to trim strings
static std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, (last - first + 1));
}

void ScriptManager::scan_scripts(const fs::path& scripts_dir) {
    m_scripts.clear();
    if (!fs::is_directory(scripts_dir)) return;

    LOG_INFO("Scanning for scripts in ", scripts_dir.string());
    for (const auto& entry : fs::directory_iterator(scripts_dir)) {
        if (!fs::is_regular_file(entry)) continue;

        std::ifstream file(entry.path().string());
        if (!file.is_open()) continue;

        ScriptDefinition script;
        script.script_path = entry.path();
        script.title = entry.path().stem().string(); // Default title

        std::string line;
        bool in_comment_block = false;
        
        // Read shebang
        std::getline(file, line);
        if (line.rfind("#!", 0) != 0) continue; // Not a script

        while (std::getline(file, line)) {
            std::string trimmed_line = trim(line);
            if (trimmed_line.empty()) {
                if(in_comment_block) break; // End of metadata block
                else continue;
            }

            // Check for start of comment
            bool is_comment = (trimmed_line.rfind("#", 0) == 0 || trimmed_line.rfind("//", 0) == 0);
            if (is_comment) {
                in_comment_block = true;
                size_t start = (trimmed_line[1] == '/') ? 2 : 1;
                std::string content = trim(trimmed_line.substr(start));
                
                size_t space_pos = content.find_first_of(" \t");
                if (space_pos == std::string::npos) continue;

                std::string key = content.substr(0, space_pos);
                std::string value = trim(content.substr(space_pos + 1));
                
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);

                if (key == "TITLE:") script.title = value;
                else if (key == "AUTHOR:") script.author = value;
                else if (key == "DESCRIPTION:") script.description = value;
                else if (key == "ARGS:") script.args_template = value;
                else if (key == "HAS_OUTPUT:") script.has_output = (value == "TRUE");
                else if (key == "HAS_PROGRESS:") script.has_progress = (value == "TRUE");
                else if (key == "CAN_CANCEL:") script.can_cancel = (value == "TRUE");
                else if (key == "PRIORITY:") {
                    try { script.priority = std::stoi(value); }
                    catch (...) { /* ignore invalid number */ }
                }
                else if (key == "REGISTER:") {
                    if (value == "RUN_BEFORE_LAUNCH") script.registration = ScriptRegistration::RUN_BEFORE_LAUNCH;
                    else if (value == "SORT_DATA") script.registration = ScriptRegistration::SORT_DATA;
                    else if (value == "SORT_CONTENT") script.registration = ScriptRegistration::SORT_CONTENT;
                    else if (value == "VERIFY") script.registration = ScriptRegistration::VERIFY;
                }
                else if (key.find("_TYPE:") != std::string::npos) {
                    std::string opt_name = key.substr(0, key.find("_TYPE:"));
                    ScriptOption* opt = nullptr;
                    for (auto& o : script.config_options) if (o.name == opt_name) opt = &o;
                    if (!opt) {
                        script.config_options.emplace_back();
                        opt = &script.config_options.back();
                        opt->name = opt_name;
                    }
                    if (value == "SELECT") opt->type = ScriptOptionType::SELECT;
                    else if (value == "CHECKBOX") opt->type = ScriptOptionType::CHECKBOX;
                }
                else if (key.find("_OPTIONS:") != std::string::npos) {
                     std::string opt_name = key.substr(0, key.find("_OPTIONS:"));
                     for (auto& opt : script.config_options) if (opt.name == opt_name) {
                        std::stringstream ss(value); std::string item;
                        while(std::getline(ss, item, ';')) opt.options.push_back(item);
                        break;
                     }
                }
                else if (key.find("_DEFAULT:") != std::string::npos) {
                     std::string opt_name = key.substr(0, key.find("_DEFAULT:"));
                     for (auto& opt : script.config_options) if (opt.name == opt_name) {
                        opt.default_value = value;
                        opt.current_value = value; // Set current to default initially
                        break;
                     }
                }
            } else if (in_comment_block) {
                break; // End of metadata
            }
        }
        m_scripts.push_back(script);
    }
    LOG_INFO("Found ", m_scripts.size(), " scripts.");

    // --- NEW: Auto-select first available sorter if none is set ---
    if (m_active_data_sorter.empty()) {
        auto sorters = get_scripts_by_registration(ScriptRegistration::SORT_DATA);
        if (!sorters.empty()) {
            m_active_data_sorter = sorters[0]->script_path;
            LOG_INFO("No active data sorter set. Defaulting to: ", m_active_data_sorter.filename().string());
        }
    }
    if (m_active_content_sorter.empty()) {
        auto sorters = get_scripts_by_registration(ScriptRegistration::SORT_CONTENT);
        if (!sorters.empty()) {
            m_active_content_sorter = sorters[0]->script_path;
            LOG_INFO("No active content sorter set. Defaulting to: ", m_active_content_sorter.filename().string());
        }
    }
    if (m_active_content_verifier.empty()) {
        auto verifiers = get_scripts_by_registration(ScriptRegistration::VERIFY);
        if (!verifiers.empty()) {
            m_active_content_verifier = verifiers[0]->script_path;
            LOG_INFO("No active content verifier set. Defaulting to: ", m_active_content_verifier.filename().string());
        }
    }
}

void ScriptManager::load_options(const fs::path& options_file) {
    m_options_file = options_file;
    std::ifstream file(options_file.string());
    if (!file.is_open()) return;

    LOG_INFO("Loading script options from ", options_file.string());
    std::string line, current_section;
    while(std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if(line.front() == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
        } else {
            size_t eq_pos = line.find('=');
            if (eq_pos == std::string::npos) continue;
            std::string key = trim(line.substr(0, eq_pos));
            std::string val = trim(line.substr(eq_pos + 1));

            if (current_section == "config") { // NEW: Handle sorters
                LOG_INFO("Script Config: ", key, "=", val);
                if (key == "active_data_sorter") {
                    // Find the script with this filename and store its full path
                    for (const auto& script : m_scripts) {
                        if (script.script_path.filename().string() == val) {
                            m_active_data_sorter = script.script_path;
                            break;
                        }
                    }
                }
                if (key == "active_content_sorter") {
                    for (const auto& script : m_scripts) {
                        if (script.script_path.filename().string() == val) {
                            m_active_content_sorter = script.script_path;
                            break;
                        }
                    }
                }
                if (key == "active_content_verifier") {
                     for (const auto& script : m_scripts) {
                        if (script.script_path.filename().string() == val) {
                            m_active_content_verifier = script.script_path;
                            break;
                        }
                    }
                }
            } else if (current_section == "scripts") { // Handle enabled status
                for (auto& script : m_scripts) {
                    if (script.script_path.filename().string() == key) {
                        script.enabled = (val == "true");
                        break;
                    }
                }
            } else if (current_section.rfind(".options", current_section.length() - 8) != std::string::npos) {
                std::string script_name = current_section.substr(0, current_section.length() - 8);
                for (auto& script : m_scripts) {
                    if (script.script_path.filename().string() == script_name) {
                        for (auto& opt : script.config_options) {
                            if (opt.name == key) {
                                opt.current_value = val;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void ScriptManager::save_options(const fs::path& options_file) {
    std::ofstream file(options_file.string());
    if (!file.is_open()) {
        LOG_ERROR("Could not save script options to ", options_file.string());
        return;
    }

    LOG_INFO("Saving script options to ", options_file.string());
    
    // --- NEW: Write config section ---
    file << "[config]\n";
    if (!m_active_data_sorter.empty()) file << "active_data_sorter=" << m_active_data_sorter.filename().string() << "\n";
    if (!m_active_content_sorter.empty()) file << "active_content_sorter=" << m_active_content_sorter.filename().string() << "\n";
    if (!m_active_content_verifier.empty()) file << "active_content_verifier=" << m_active_content_verifier.filename().string() << "\n";
    file << "\n";

    file << "[scripts]\n";
    for (const auto& script : m_scripts) {
        file << script.script_path.filename().string() << "=" << (script.enabled ? "true" : "false") << "\n";
    }
    file << "\n";

    for (const auto& script : m_scripts) {
        if (script.config_options.empty()) continue;
        file << "[" << script.script_path.filename().string() << ".options]\n";
        for (const auto& opt : script.config_options) {
            file << opt.name << "=" << opt.current_value << "\n";
        }
        file << "\n";
    }
}


std::string ScriptManager::build_command_string(
    ScriptDefinition& script,
    const AppContext& ctx,
    const std::map<std::string, std::string>& extra_vars
) {
    std::string command = "\"" + script.script_path.string() + "\"";
    std::string args = script.args_template;
    
    std::map<std::string, std::string> variables;
    // Base variables
    variables["$OPENMW_CFG"] = ctx.path_openmw_cfg.string();
    variables["$OPENMW_ESMM_INI"] = (ctx.path_config_dir / "openmw_esmm.ini").string();
    variables["$OPENMW_DIR"] = ctx.path_config_dir.string();
    variables["$MOD_DATA"] = ctx.path_mod_data.string();

    // Script-defined options
    for (const auto& opt : script.config_options) {
        variables["$" + opt.name] = opt.current_value;
    }

    // Extra variables (e.g., from a runner)
    for (const auto& pair : extra_vars) {
        variables[pair.first] = pair.second;
    }

    for (const auto& pair : variables) {
        size_t pos = args.find(pair.first);
        while (pos != std::string::npos) {
            args.replace(pos, pair.first.length(), "\"" + pair.second + "\""); // Always quote paths
            pos = args.find(pair.first, pos + pair.second.length() + 2);
        }
    }

    command += " " + args; // stderr is handled by the runner
    return command;
}


ScriptDefinition* ScriptManager::get_script_by_path(const fs::path& script_path) {
    for (auto& script : m_scripts) {
        if (script.script_path == script_path) return &script;
    }
    return nullptr;
}


std::vector<ScriptDefinition*> ScriptManager::get_scripts_by_registration(ScriptRegistration registration) {
    std::vector<ScriptDefinition*> result;

    for (auto& script : m_scripts) {
        if (script.registration == registration) result.push_back(&script);
    }

    std::sort(result.begin(), result.end(), [](const ScriptDefinition* a, const ScriptDefinition* b) {
        return a->priority < b->priority;
    });

    return result;
}



void ScriptManager::set_active_data_sorter_path(const fs::path& script_path) {
    m_active_data_sorter = script_path;
}


void ScriptManager::set_active_content_sorter_path(const fs::path& script_path) {
    m_active_content_sorter = script_path;
}

void ScriptManager::set_active_content_verifier_path(const fs::path& script_path) {
    m_active_content_verifier = script_path;
}
