#include "ModEngine.h"
#include "StateMachine.h"
#include "../mod/ConfigParser.h"
#include "../scenes/ScriptRunner.h"
#include "../scenes/ScriptRunnerScene.h"
#include "../utils/Logger.h"
#include "../utils/Utils.h"
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

StateMachine& ModEngine::get_state_machine() { return *m_state_machine; }
void ModEngine::set_state_machine(StateMachine& machine) { m_state_machine = &machine; }

void ModEngine::discover_mod_definitions() {
    // This method contains the core logic for finding and parsing mod directories.
    const auto& loaded_cfg = m_config_manager.get_loaded_data();

    m_mod_manager.mod_definitions.clear();
    std::vector<fs::path> all_data_paths = loaded_cfg.data_paths;
    
    fs::path game_data_path;
    all_data_paths.erase(std::remove_if(all_data_paths.begin(), all_data_paths.end(),
        [&](const fs::path& p) {
            if (fs::exists(p / "Morrowind.esm")) {
                game_data_path = p;
                return true;
            }
            return false;
        }), all_data_paths.end());

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
    
    if (fs::exists(m_app_context.path_mod_data)) {
        for (const auto& entry : fs::directory_iterator(m_app_context.path_mod_data)) {
            if (fs::is_directory(entry)) all_data_paths.push_back(entry.path());
        }
    }

    std::map<fs::path, std::vector<fs::path>> grouped_paths;
    for (const auto& path : all_data_paths) {
        fs::path mod_root;
        for (const auto& source_dir : m_mod_source_dirs) {
            if (!source_dir.empty() && path.string().find(source_dir.string()) == 0) {
                fs::path relative = path.lexically_relative(source_dir);
                if (!relative.empty() && relative.string() != ".") {
                    mod_root = source_dir / *relative.begin();
                    break;
                }
            }
        }
        if (!mod_root.empty()) grouped_paths[mod_root].push_back(path);
    }
    
    for (const auto& pair : grouped_paths) {
        if (fs::exists(pair.first)) {
            m_mod_manager.mod_definitions.push_back(parse_mod_directory(pair.first));
        }
    }

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
}


void ModEngine::rescan_mods() {
    LOG_INFO("Rescanning installed mods...");
    discover_mod_definitions();
    m_mod_manager.sync_ui_state_from_active_lists(); // Use the renamed function
    LOG_INFO("Mod rescan complete.");
}

void ModEngine::initialize() {
    if (m_is_initialized) return;
    LOG_INFO("ModEngine: Initializing...");

    // 1. Load support systems
    m_script_manager.scan_scripts(m_app_context.path_config_dir / "scripts");
    m_script_manager.load_options(m_app_context.path_config_dir / "openmw_esmm_options.cfg");
    m_config_manager.load(m_app_context.path_openmw_cfg);
    
    // 2. Discover all available mods from the filesystem. This populates m_mod_manager.mod_definitions
    discover_mod_definitions();

    // 3. Set the active lists in ModManager directly from the loaded config. This is the source of truth.
    const auto& loaded_cfg = m_config_manager.get_loaded_data();
    m_mod_manager.active_data_paths = loaded_cfg.data_paths;
    m_mod_manager.active_content_files = loaded_cfg.content_files;

    // 4. Enrich the active_content_files with source_mod info found during discovery
    std::map<std::string, std::string> all_plugins_map;
    for(const auto& mod : m_mod_manager.mod_definitions) {
        for(const auto& group : mod.option_groups) for(const auto& option : group.options) for(const auto& plugin : option.discovered_plugins) {
            all_plugins_map[plugin] = mod.name;
        }
    }
    for (auto& cf : m_mod_manager.active_content_files) {
        if (all_plugins_map.count(cf.name)) {
            cf.source_mod = all_plugins_map[cf.name];
        }
    }

    // 5. NOW, sync the UI state (checkboxes) based on the true active lists we just established
    m_mod_manager.sync_ui_state_from_active_lists();

    // 6. Scan archives and perform initial sort
    m_archive_manager.scan_archives(m_app_context.path_mod_archives, m_app_context.path_mod_data);
    LOG_INFO("Performing initial sort of core game files...");

    auto& data_paths = m_mod_manager.active_data_paths;
    auto it_data = std::find_if(data_paths.begin(), data_paths.end(), [](const fs::path& p) {
        return fs::exists(p / "Morrowind.esm");
    });
    if (it_data != data_paths.end() && it_data != data_paths.begin()) {
        fs::path gdp = *it_data;
        data_paths.erase(it_data);
        data_paths.insert(data_paths.begin(), gdp);
    }

    auto& content_files = m_mod_manager.active_content_files;
    const std::vector<std::string> masters = {"Bloodmoon.esm", "Tribunal.esm", "Morrowind.esm"};
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


bool ModEngine::write_temporary_cfg(const fs::path& temp_cfg_path) {
    std::ifstream original_file(m_app_context.path_openmw_cfg.string());
    if (!original_file.is_open()) {
        LOG_ERROR("Could not open original openmw.cfg to create temporary copy.");
        return false;
    }

    std::ofstream temp_file(temp_cfg_path.string());
    if (!temp_file.is_open()) {
        LOG_ERROR("Could not create temporary openmw.cfg at ", temp_cfg_path.string());
        return false;
    }

    // This logic is a simplified version of ConfigManager::save
    bool data_written = false;
    bool content_written = false;
    std::string line;

    while (std::getline(original_file, line)) {
        std::string trimmed_line = trim(line);
        if (trimmed_line.rfind("data=", 0) == 0) {
            if (!data_written) {
                fs::path original_cfg_dir = m_app_context.path_openmw_cfg.parent_path();
                for (const auto& path : m_mod_manager.active_data_paths) {
                    fs::path abs_path = fs::absolute(path, original_cfg_dir);
                    temp_file << "data=\"" << abs_path.string() << "\"\n";
                }
                data_written = true;
            }
        } else if (trimmed_line.rfind("content=", 0) == 0 || trimmed_line.rfind("#content=", 0) == 0) {
            if (!content_written) {
                for (const auto& cf : m_mod_manager.active_content_files) {
                    if (cf.enabled) temp_file << "content=\"" << cf.name << "\"\n";
                    else temp_file << "#content=\"" << cf.name << "\"\n";
                }
                content_written = true;
            }
        } else {
            temp_file << line << "\n";
        }
    }
    return true;
}

void ModEngine::delete_mod_data(const std::vector<fs::path>& paths_to_delete) {
    for (const auto& path : paths_to_delete) {
        if (fs::exists(path)) {
            fs::remove_all(path);
        }
    }
}


void ModEngine::save_configuration() {
    LOG_INFO("Saving configuration to ", m_app_context.path_openmw_cfg.string());
    ConfigData data_to_save;
    data_to_save.data_paths = m_mod_manager.active_data_paths;
    data_to_save.content_files = m_mod_manager.active_content_files;
    m_config_manager.save(m_app_context.path_openmw_cfg, data_to_save);
}


void ModEngine::run_active_sorter(ScriptRegistration type) {
    fs::path sorter_path = (type == ScriptRegistration::SORT_DATA)
        ? m_script_manager.get_active_data_sorter_path()
        : m_script_manager.get_active_content_sorter_path();

    if (sorter_path.empty()) return;
    ScriptDefinition* script = m_script_manager.get_script_by_path(sorter_path);
    if (!script) return;

    HeadlessScriptRunner runner(*m_state_machine, *script);
    runner.run({}, true);
    const ScriptRunResult& result = runner.get_result();

    // This is the variable we need to clean up
    fs::path temp_dir_to_cleanup;
    if (result.modified_cfg_path) {
        temp_dir_to_cleanup = result.modified_cfg_path->parent_path();
    }
    
    if (result.return_code == 0 && result.modified_cfg_path) {
        LOG_INFO("Sorter script succeeded. Applying changes.");
        auto new_config_data_ptr = ConfigParser::read_config(*result.modified_cfg_path);
        if (new_config_data_ptr) {
            const auto& new_config_data = *new_config_data_ptr;

            LOG_INFO("--- Parsed Data Paths from Temp Config ---");
            for(const auto& p : new_config_data.data_paths) LOG_INFO(p.string());

            LOG_INFO("--- Data Load Order BEFORE Applying ---");
            for(const auto& p : m_mod_manager.active_data_paths) LOG_INFO(p.string());

            std::map<fs::path, fs::path> abs_to_original_path_map;
            fs::path original_cfg_dir = m_app_context.path_openmw_cfg.parent_path();
            for (const auto& original_path : m_mod_manager.active_data_paths) {
                abs_to_original_path_map[fs::absolute(original_path, original_cfg_dir)] = original_path;
            }

            std::vector<fs::path> new_sorted_data_paths;
            for (const auto& new_abs_path : new_config_data.data_paths) {
                if (abs_to_original_path_map.count(new_abs_path)) {
                    new_sorted_data_paths.push_back(abs_to_original_path_map[new_abs_path]);
                } else {
                    new_sorted_data_paths.push_back(new_abs_path);
                }
            }
            m_mod_manager.active_data_paths = new_sorted_data_paths;
            
            std::map<std::string, ContentFile> old_cf_map;
            for(const auto& cf : m_mod_manager.active_content_files) old_cf_map[cf.name] = cf;

            std::vector<ContentFile> final_content_files;
            for(const auto& new_cf : new_config_data.content_files) {
                final_content_files.push_back(ContentFile{
                    new_cf.name, 
                    new_cf.enabled, 
                    old_cf_map.count(new_cf.name) ? old_cf_map[new_cf.name].source_mod : "Unknown"
                });
            }
            m_mod_manager.active_content_files = final_content_files;

            LOG_INFO("--- Data Load Order AFTER applying ---");
            for(const auto& p : m_mod_manager.active_data_paths) LOG_INFO(p.string());

            m_mod_manager.sync_ui_state_from_active_lists();
        }
    } else {
        LOG_ERROR("Sorter script failed with code ", result.return_code, ". No changes applied.");
    }
    
    // Correctly clean up the temp directory
    if (!temp_dir_to_cleanup.empty() && fs::exists(temp_dir_to_cleanup)) {
        fs::remove_all(temp_dir_to_cleanup);
    }
}


void ModEngine::run_active_verifier() {
    fs::path verifier_path = m_script_manager.get_active_content_verifier_path();
    if (verifier_path.empty()) return;
    ScriptDefinition* script = m_script_manager.get_script_by_path(verifier_path);
    if (!script) return;
    
    // Verifier needs a temp config, but also a UI.
    if (script->has_output) {
        auto runner = std::make_shared<UIScriptRunner>(*m_state_machine, *script);
        auto scene = std::make_unique<ScriptRunnerScene>(*m_state_machine, runner, *script, true); // Pass use_temp_cfg=true
        m_state_machine->push_scene(std::move(scene));
    } else {
        HeadlessScriptRunner runner(*m_state_machine, *script);
        runner.run({}, true);
    }
}


bool ModEngine::has_active_sorter(ScriptRegistration type) const {
    if (type == ScriptRegistration::SORT_DATA) {
        return !m_script_manager.get_active_data_sorter_path().empty();
    }
    if (type == ScriptRegistration::SORT_CONTENT) {
        return !m_script_manager.get_active_content_sorter_path().empty();
    }
    return false;
}

bool ModEngine::has_active_verifier() const {
    return !m_script_manager.get_active_content_verifier_path().empty();
}

void ModEngine::add_running_script(ScriptRunner* runner) {
    m_running_scripts.insert(runner);
}

void ModEngine::remove_running_script(ScriptRunner* runner) {
    m_running_scripts.erase(runner);
}

const std::set<ScriptRunner*>& ModEngine::get_running_scripts() const {
    return m_running_scripts;
}
