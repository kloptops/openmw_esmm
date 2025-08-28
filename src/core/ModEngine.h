#pragma once
#include "../mod/ModManager.h"
#include "../mod/ArchiveManager.h"
#include "../mod/ConfigManager.h"
#include "../mod/MloxManager.h"
#include "../mod/SortManager.h"
#include "../AppContext.h"
#include <vector>
#include <string>

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

    void save_configuration();
    void sort_data_paths_by_rules();
    void sort_content_files_by_mlox();
    void delete_mod_data(const std::vector<fs::path>& paths_to_delete);

    const ModManager& get_mod_manager() const { return m_mod_manager; }
    const ArchiveManager& get_archive_manager() const { return m_archive_manager; }
    const MloxManager& get_mlox_manager() const { return m_mlox_manager; }

    ArchiveManager& get_archive_manager_mut() { return m_archive_manager; }
    ModManager& get_mod_manager_mut() { return m_mod_manager; }
    SortManager& get_sort_manager_mut() { return m_sort_manager; }
    MloxManager& get_mlox_manager_mut() { return m_mlox_manager; }
    ConfigManager& get_config_manager_mut() { return m_config_manager; }

    // Are any rules loaded?
    bool mlox_rules_loaded() { return m_mlox_manager.rules_loaded(); }
    bool sort_rules_loaded() { return m_sort_manager.rules_loaded(); }

private:
    // Now a reference, not a copy!
    AppContext& m_app_context;
    ModManager m_mod_manager;
    ArchiveManager m_archive_manager;
    ConfigManager m_config_manager;
    MloxManager m_mlox_manager;
    SortManager m_sort_manager;
    bool m_is_initialized = false;
    std::vector<fs::path> m_mod_source_dirs; // NEW MEMBER
};
