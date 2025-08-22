#include "ModEngine.h"
#include "../utils/Logger.h"
#include <set>
#include <vector>
#include <algorithm>
#include <map>

// Helper to find the common base path for a list of paths
static fs::path find_common_base(const std::vector<fs::path>& paths) {
    if (paths.empty()) return "";
    if (paths.size() == 1) return paths[0].parent_path();

    fs::path common_path = paths[0].parent_path();
    for (size_t i = 1; i < paths.size(); ++i) {
        // while path[i] is not a descendant of common_path
        while (paths[i].string().find(common_path.string()) != 0 || paths[i] == common_path) {
            if (common_path.has_parent_path()) {
                common_path = common_path.parent_path();
            } else {
                return ""; // No common base found
            }
        }
    }
    return common_path;
}

ModEngine::ModEngine(AppContext& ctx) : m_app_context(ctx) {}

void ModEngine::initialize() {
    if (m_is_initialized) return;

    LOG_INFO("ModEngine: Initializing...");

    // --- Load Rules and Base Config ---
    fs::path ini_path = m_app_context.path_config_dir / "openmw_esmm.ini";
    fs::path base_mlox_path = m_app_context.path_config_dir / "mlox_base.txt";
    fs::path user_mlox_path = m_app_context.path_config_dir / "mlox_user.txt";
    m_sort_manager.load_rules(ini_path);
    m_mlox_manager.load_rules({base_mlox_path, user_mlox_path});
    m_config_manager.load(m_app_context.path_openmw_cfg);

    // --- NEW, ROBUST MOD DISCOVERY LOGIC ---
    m_mod_manager.mod_definitions.clear();
    std::vector<fs::path> all_data_paths;
    for (const auto& p_str : m_config_manager.data_paths) {
        if (!p_str.empty()) all_data_paths.emplace_back(p_str);
    }

    // 1. Find and separate the base game data path
    fs::path game_data_path;
    all_data_paths.erase(std::remove_if(all_data_paths.begin(), all_data_paths.end(),
        [&](const fs::path& p) {
            if (fs::exists(p / "Morrowind.esm")) {
                game_data_path = p;
                return true;
            }
            return false;
        }), all_data_paths.end());

    // 2. Group remaining paths by their logical mod root
    m_mod_source_dirs.clear();
    if (fs::exists(m_app_context.path_mod_data)) {
        m_mod_source_dirs.push_back(m_app_context.path_mod_data);
    }
    
    std::vector<fs::path> external_paths;
    for(const auto& p : all_data_paths) {
        if (p.string().find(m_app_context.path_mod_data.string()) != 0) {
            external_paths.push_back(p);
        }
    }
    if (!external_paths.empty()) {
        m_mod_source_dirs.push_back(find_common_base(external_paths));
    }

    std::map<fs::path, std::vector<fs::path>> grouped_paths;
    for (const auto& path : all_data_paths) {
        fs::path mod_root;
        for (const auto& source_dir : m_mod_source_dirs) {
            if (!source_dir.empty() && path.string().find(source_dir.string()) == 0) {
                fs::path relative = path.lexically_relative(source_dir);
                if (!relative.empty()) {
                    mod_root = source_dir / *relative.begin(); // The first directory component is the mod name
                    break;
                }
            }
        }
        if (!mod_root.empty()) {
            grouped_paths[mod_root].push_back(path);
        }
    }

    // 3. Parse each identified mod root
    for (const auto& pair : grouped_paths) {
        if (fs::exists(pair.first)) {
            m_mod_manager.mod_definitions.push_back(parse_mod_directory(pair.first));
        }
    }

    // 4. Manually create the ModDefinition for the base game
    if (!game_data_path.empty()) {
        ModDefinition mw_mod;
        mw_mod.name = "The Elder Scrolls III: Morrowind";
        mw_mod.root_path = game_data_path;
        mw_mod.enabled = true;

        ModOptionGroup main_group;
        main_group.name = "Main";
        main_group.type = ModOptionGroup::MULTIPLE_CHOICE;
        main_group.required = true;
        
        ModOption main_option;
        main_option.name = "Data Files";
        main_option.path = game_data_path;
        main_option.enabled = true;
        main_option.parent_group = &main_group;

        main_option.discovered_plugins = find_plugins_in_path(game_data_path);

        main_group.options.push_back(main_option);

        mw_mod.option_groups.push_back(main_group);
        m_mod_manager.mod_definitions.insert(m_mod_manager.mod_definitions.begin(), mw_mod);
    }

    std::sort(m_mod_manager.mod_definitions.begin(), m_mod_manager.mod_definitions.end(),
        [](const ModDefinition& a, const ModDefinition& b) {
            if (a.name == "The Elder Scrolls III: Morrowind") return true;
            if (b.name == "The Elder Scrolls III: Morrowind") return false;
            return a.name < b.name;
        });

    // --- Sync state and finish initialization as before ---
    std::vector<std::string> all_content_for_sync = m_config_manager.content_files;
    all_content_for_sync.insert(all_content_for_sync.end(), m_config_manager.disabled_content_files.begin(), m_config_manager.disabled_content_files.end());
    m_mod_manager.sync_state_from_config(m_config_manager.data_paths, all_content_for_sync);
    
    m_mod_manager.active_data_paths.clear();
    for (const auto& p_str : m_config_manager.data_paths) m_mod_manager.active_data_paths.push_back(fs::path(p_str));
    
    m_mod_manager.active_content_files.clear();
    std::map<std::string, std::string> all_plugins_map;
    // Add base game plugins
    if (!game_data_path.empty() && fs::is_directory(game_data_path)) {
        for(const auto& entry : fs::directory_iterator(game_data_path)) {
            if (is_plugin_file(entry.path())) {
                all_plugins_map[entry.path().filename().string()] = "The Elder Scrolls III: Morrowind";
            }
        }
    }
    // Add mod plugins
    for(const auto& mod : m_mod_manager.mod_definitions) {
        for(const auto& group : mod.option_groups) for(const auto& option : group.options) for(const auto& plugin : option.discovered_plugins) all_plugins_map[plugin] = mod.name;
    }
    
    std::set<std::string> enabled_set(m_config_manager.content_files.begin(), m_config_manager.content_files.end());
    for (const auto& c_str : all_content_for_sync) {
        std::string source_mod = all_plugins_map.count(c_str) ? all_plugins_map[c_str] : "Unknown";
        m_mod_manager.active_content_files.push_back({c_str, enabled_set.count(c_str) > 0, source_mod});
    }
    m_mod_manager.update_active_lists();
    
    m_archive_manager.scan_archives(m_app_context.path_mod_archives, m_app_context.path_mod_data);

    LOG_INFO("Performing initial sort of core game files...");

    // 1. Force the main data path to the top of the data list.
    auto& data_paths = m_mod_manager.active_data_paths;
    auto it_data = std::find_if(data_paths.begin(), data_paths.end(), [](const fs::path& p) {
        return fs::exists(p / "Morrowind.esm");
    });
    if (it_data != data_paths.end() && it_data != data_paths.begin()) {
        fs::path game_data_path = *it_data;
        data_paths.erase(it_data);
        data_paths.insert(data_paths.begin(), game_data_path);
    }

    // 2. Force the core ESMs to the top of the content list in the correct order.
    auto& content_files = m_mod_manager.active_content_files;
    const std::vector<std::string> masters = {"Bloodmoon.esm", "Tribunal.esm", "Morrowind.esm"}; // Process in reverse for insertion at the front
    for (const auto& master_name : masters) {
        auto it_content = std::find_if(content_files.begin(), content_files.end(), [&](const ContentFile& cf){
            return cf.name == master_name;
        });
        if (it_content != content_files.end()) {
            ContentFile master_file = *it_content;
            content_files.erase(it_content);
            content_files.insert(content_files.begin(), master_file);
        }
    }

    m_is_initialized = true;

    LOG_INFO("ModEngine: Initialization complete.");
}

void ModEngine::rescan_archives() {
    m_archive_manager.scan_archives(m_app_context.path_mod_archives, m_app_context.path_mod_data);
}

void ModEngine::save_configuration() {
    LOG_INFO("Saving configuration to ", m_app_context.path_openmw_cfg.string());
    
    m_config_manager.data_paths.clear();
    for (const auto& path : m_mod_manager.active_data_paths) m_config_manager.data_paths.push_back(path.string());
    
    m_config_manager.content_files.clear();
    m_config_manager.disabled_content_files.clear();
    for (const auto& cf : m_mod_manager.active_content_files) {
        if (cf.enabled) m_config_manager.content_files.push_back(cf.name);
        else m_config_manager.disabled_content_files.push_back(cf.name);
    }
    
    m_config_manager.save(m_app_context.path_openmw_cfg);
}

void ModEngine::sort_data_paths_by_rules() {
    m_sort_manager.sort_data_paths(m_mod_manager.active_data_paths, m_mod_source_dirs);
}

void ModEngine::sort_content_files_by_mlox() {
    std::vector<fs::path> base_paths; // mlox now gets all paths
    for (const auto& p : m_mod_manager.active_data_paths) base_paths.emplace_back(p);
    m_mlox_manager.sort_content_files(m_mod_manager.active_content_files, base_paths, {}); // base_paths now contains everything
}

void ModEngine::delete_mod_data(const std::vector<fs::path>& paths_to_delete) {
    for (const auto& path : paths_to_delete) {
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    }
}
