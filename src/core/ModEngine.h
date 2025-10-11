#pragma once
#include "../mod/ModManager.h"
#include "../mod/ArchiveManager.h"
#include "../mod/ConfigManager.h"
#include "../mod/ScriptManager.h"
#include "../AppContext.h"
#include <vector>
#include <string>
#include <set>

// Forward declare this to avoid a circular reference.
class ScriptRunner;
class StateMachine;

// Forward declarations
ModDefinition parse_mod_directory(const fs::path& mod_root_path);
bool is_plugin_file(const fs::path& path);

class ModEngine {
public:
    // Constructor now requires the AppContext
    ModEngine(AppContext& ctx);
    
    // Initialize no longer needs the context passed in
    void initialize();
    void rescan_archives();
    void rescan_mods();

    void run_active_sorter(ScriptRegistration type);
    void run_active_verifier();

    bool has_active_sorter(ScriptRegistration type) const;
    bool has_active_verifier() const;

    void save_configuration();
    bool write_temporary_cfg(const fs::path& temp_cfg_path);

    void delete_mod_data(const std::vector<fs::path>& paths_to_delete);

    const ModManager& get_mod_manager() const { return m_mod_manager; }
    const ArchiveManager& get_archive_manager() const { return m_archive_manager; }

    ArchiveManager& get_archive_manager_mut() { return m_archive_manager; }
    ModManager& get_mod_manager_mut() { return m_mod_manager; }
    ScriptManager* get_script_manager_mut() { return &m_script_manager; } // NEW
    ConfigManager& get_config_manager_mut() { return m_config_manager; }

    StateMachine& get_state_machine();
    void set_state_machine(StateMachine& machine);

    void add_running_script(ScriptRunner* runner);
    void remove_running_script(ScriptRunner* runner);
    const std::set<ScriptRunner*>& get_running_scripts() const;

private:
    void discover_mod_definitions();

    StateMachine* m_state_machine = nullptr;

    AppContext& m_app_context;
    ModManager m_mod_manager;
    ArchiveManager m_archive_manager;
    ConfigManager m_config_manager;
    ScriptManager m_script_manager;

    bool m_is_initialized = false;
    std::vector<fs::path> m_mod_source_dirs;

    std::set<ScriptRunner*> m_running_scripts;
};
