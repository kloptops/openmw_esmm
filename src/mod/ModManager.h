#pragma once
#include <string>
#include <vector>
#include <map>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

std::vector<std::string> find_plugins_in_path(const fs::path& path);

// Forward declare to resolve circular dependency
struct ModOptionGroup;

// Represents a single configurable choice within a mod
struct ModOption {
    std::string name;
    fs::path path;
    std::vector<std::string> discovered_plugins; // An option can enable multiple plugins
    bool enabled = false;
    ModOptionGroup* parent_group = nullptr; // Pointer back to the parent group
};

// A group of options, often mutually exclusive
struct ModOptionGroup {
    enum Type { SINGLE_CHOICE, MULTIPLE_CHOICE };
    std::string name;
    Type type;
    std::vector<ModOption> options;
    bool required = false;
};

// Represents a whole mod as discovered in a single directory
struct ModDefinition {
    std::string name;
    fs::path root_path;
    std::vector<ModOptionGroup> option_groups;
    std::vector<ModOption> standalone_plugins;
    bool enabled = false;
};

// A flattened structure for easy UI rendering and interaction
struct ModDisplayItem {
    enum ItemType { MOD, GROUP, OPTION, PLUGIN };
    ItemType type;
    int indent = 0;
    void* ptr = nullptr; // A void* pointer back to the original ModDefinition, ModOptionGroup, etc.
};

// --- NEW STRUCT for managing content files ---
struct ContentFile {
    std::string name;
    bool enabled = true;
    std::string source_mod; // Helpful for display
    bool is_new = false;    // To flag newly discovered plugins in the UI
};

// The main class that holds all state and logic
class ModManager {
public:
    void scan_mods(const fs::path& mod_data_path);
    void sync_ui_state_from_active_lists();
    void update_active_lists();

    std::vector<ModDefinition> mod_definitions;

    // The final, reorderable lists for the config file
    std::vector<fs::path> active_data_paths;
    std::vector<ContentFile> active_content_files; // CHANGED to the new struct
};
