#pragma once
#include "../AppContext.h"
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum class ScriptRegistration {
    RUN_BEFORE_LAUNCH,
    SORT_DATA,
    SORT_CONTENT,
    VERIFY,
    NONE
};

enum class ScriptOptionType {
    SELECT,
    CHECKBOX,
    UNKNOWN
};

struct ScriptOption {
    std::string name;
    ScriptOptionType type = ScriptOptionType::UNKNOWN;
    std::vector<std::string> options; // For SELECT
    std::string default_value;
    std::string current_value;
};

struct ScriptDefinition {
    fs::path script_path;
    std::string title = "Untitled Script";
    std::string author = "Unknown";
    std::string description;
    std::string args_template;
    bool has_output = true;
    bool has_progress = false;
    bool can_cancel = false;
    ScriptRegistration registration = ScriptRegistration::NONE;
    bool enabled = true;
    int priority = 1000;

    std::vector<ScriptOption> config_options;
};

class ScriptManager {
public:
    void scan_scripts(const fs::path& scripts_dir);
    void load_options(const fs::path& options_file);
    void save_options(const fs::path& options_file);

    std::string build_command_string(
        ScriptDefinition& script,
        const AppContext& ctx,
        const std::map<std::string, std::string>& extra_vars = {} // NEW
    );

    const std::vector<ScriptDefinition>& get_all_scripts() const { return m_scripts; }
    std::vector<ScriptDefinition>& get_all_scripts_mut() { return m_scripts; } // NEW

    ScriptDefinition* get_script_by_path(const fs::path& script_path);
    std::vector<ScriptDefinition*> get_scripts_by_registration(ScriptRegistration registration);

    fs::path get_active_data_sorter_path() const { return m_active_data_sorter; }
    fs::path get_active_content_sorter_path() const { return m_active_content_sorter; }
    fs::path get_active_content_verifier_path() const { return m_active_content_verifier; }
    void set_active_data_sorter_path(const fs::path& script_path);
    void set_active_content_sorter_path(const fs::path& script_path);
    void set_active_content_verifier_path(const fs::path& script_path);

private:
    std::vector<ScriptDefinition> m_scripts;
    fs::path m_options_file;

    fs::path m_active_data_sorter;
    fs::path m_active_content_sorter;
    fs::path m_active_content_verifier;
};

