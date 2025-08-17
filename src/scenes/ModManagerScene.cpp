#include "ModManagerScene.h"
#include "MainMenuScene.h"
#include "ExtractorScene.h"
#include "../AppContext.h"
#include "../mod/ModManager.h"
#include "../mod/ArchiveManager.h"
#include "../mod/ConfigManager.h"
#include "../mod/SortManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>

void handle_option_check(ModOption& option) {
    if (!option.enabled) return;
    if (option.parent_group && option.parent_group->type == ModOptionGroup::SINGLE_CHOICE) {
        for (auto& other_option : option.parent_group->options) {
            if (&other_option != &option) other_option.enabled = false;
        }
    }
}

// --- SIMPLIFIED STATE ---
struct ModManagerScene::State {
    ModManager mod_manager;
    ArchiveManager archive_manager;
    ConfigManager config_manager;
    SortManager sort_manager;

    bool rules_loaded = false;
    bool is_loaded = false;
    
    int focused_data_idx = -1;
    int focused_content_idx = -1;
    bool focus_request_data = false;
    bool focus_request_content = false;

    std::vector<char> archive_selection;
};

ModManagerScene::ModManagerScene() : p_state(new State()) {}
ModManagerScene::~ModManagerScene() { delete p_state; }

void ModManagerScene::on_enter(AppContext& ctx) {
    if (p_state->is_loaded) {
        // Rescan archives every time for updates
        p_state->archive_manager.scan_archives(ctx.path_mod_archives, ctx.path_mod_data);
        p_state->archive_selection.assign(p_state->archive_manager.archives.size(), 0);
        return;
    }

    fs::path ini_path = ctx.path_openmw_esmm_ini;
    p_state->rules_loaded = p_state->sort_manager.load_rules(ini_path);
    if (!p_state->rules_loaded) {
        std::cout << "Info: openmw_esmm.ini not found or failed to load. Auto-sort will be disabled." << std::endl;
    }

    p_state->mod_manager.scan_mods(ctx.path_mod_data);
    std::sort(p_state->mod_manager.mod_definitions.begin(), p_state->mod_manager.mod_definitions.end(),
        [](const ModDefinition& a, const ModDefinition& b) { return a.name < b.name; });

    fs::path config_path = ctx.path_openmw_cfg;

    if (p_state->config_manager.load(config_path)) {
        // 1. Sync the CHECKBOX states from the loaded config
        p_state->mod_manager.sync_state_from_config(p_state->config_manager.data_paths, p_state->config_manager.content_files);
        
        // 2. CRITICAL: Initialize the active lists with the EXACT order from the config file.
        p_state->mod_manager.active_data_paths.clear();
        for (const auto& p_str : p_state->config_manager.data_paths) {
            p_state->mod_manager.active_data_paths.push_back(fs::path(p_str));
        }
        
        // We need to build the ContentFile list from the config's string list
        p_state->mod_manager.active_content_files.clear();
        std::map<std::string, std::string> all_plugins; // Map all discovered plugins to their source mod
        for(const auto& mod : p_state->mod_manager.mod_definitions) {
            for(const auto& group : mod.option_groups) {
                for(const auto& option : group.options) {
                    for(const auto& plugin : option.discovered_plugins) {
                        all_plugins[plugin] = mod.name;
                    }
                }
            }
        }
        for (const auto& c_str : p_state->config_manager.content_files) {
            std::string source_mod = all_plugins.count(c_str) ? all_plugins[c_str] : "Unknown";
            p_state->mod_manager.active_content_files.push_back({c_str, true, source_mod});
        }

    } else {
        // If no config, run an initial update to populate lists with defaults
        p_state->mod_manager.update_active_lists();
    }

    p_state->archive_manager.scan_archives(ctx.path_mod_archives, ctx.path_mod_data);
    p_state->archive_selection.assign(p_state->archive_manager.archives.size(), 0);
    
    p_state->is_loaded = true;
}

void ModManagerScene::handle_event(SDL_Event& e, AppContext& ctx) { /* Empty */ }
void ModManagerScene::on_exit(AppContext& ctx) {}

// --- THE FINAL, SIMPLIFIED RENDER FUNCTION ---
void ModManagerScene::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Mod Manager", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    bool state_changed = false;

    if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
        // --- TAB 0: ARCHIVE MANAGEMENT ---
        if (ImGui::BeginTabItem("Archive Management")) {
            bool has_new_mods = false;
            bool has_updated_mods = false;
            for (const auto& archive : p_state->archive_manager.archives) {
                if (archive.status == ArchiveInfo::NEW) has_new_mods = true;
                if (archive.status == ArchiveInfo::UPDATE_AVAILABLE) has_updated_mods = true;
            }

            if (ImGui::Button("Extract/Update Selected")) {
                std::vector<ArchiveInfo> to_extract;
                for(size_t i = 0; i < p_state->archive_manager.archives.size(); ++i) {
                    if (p_state->archive_selection[i]) {
                        to_extract.push_back(p_state->archive_manager.archives[i]);
                    }
                }
                if (!to_extract.empty()) {
                    next_scene = std::make_unique<ExtractorScene>(to_extract);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Selected Mod Data")) {
                for(size_t i = 0; i < p_state->archive_manager.archives.size(); ++i) {
                    if (p_state->archive_selection[i]) {
                        const auto& archive = p_state->archive_manager.archives[i];
                        if (fs::exists(archive.target_data_path)) {
                            fs::remove_all(archive.target_data_path);
                        }
                    }
                }
                // Rescan to update status
                p_state->archive_manager.scan_archives(ctx.path_mod_archives, ctx.path_mod_data);
                p_state->archive_selection.assign(p_state->archive_manager.archives.size(), 0);
            }
            // --- NEW: Color-coded toggle buttons ---
            if (has_new_mods) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.33f, 0.8f, 0.8f)); // Green
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.33f, 0.9f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.33f, 1.0f, 1.0f));
                if (ImGui::Button("Toggle New")) {
                    for (size_t i = 0; i < p_state->archive_manager.archives.size(); ++i) {
                        if (p_state->archive_manager.archives[i].status == ArchiveInfo::NEW) {
                            p_state->archive_selection[i] = !p_state->archive_selection[i];
                        }
                    }
                }
                ImGui::PopStyleColor(3);
            }
            if (has_updated_mods) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.1f, 0.8f, 1.0f)); // Orange
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.1f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.1f, 1.0f, 1.0f));
                if (ImGui::Button("Toggle Updates")) {
                    for (size_t i = 0; i < p_state->archive_manager.archives.size(); ++i) {
                        if (p_state->archive_manager.archives[i].status == ArchiveInfo::UPDATE_AVAILABLE) {
                            p_state->archive_selection[i] = !p_state->archive_selection[i];
                        }
                    }
                }
                ImGui::PopStyleColor(3);
            }

            ImGui::Separator();

             
            ImGui::BeginChild("ArchiveList", ImVec2(0, -50), true);
            for (size_t i = 0; i < p_state->archive_manager.archives.size(); ++i) {
                auto& archive = p_state->archive_manager.archives[i];
                ImVec4 color = ImVec4(1,1,1,1);
                std::string status_text = "[Installed]";
                if (archive.status == ArchiveInfo::NEW) { color = ImVec4(0,1,0,1); status_text = "[New]"; }
                else if (archive.status == ArchiveInfo::UPDATE_AVAILABLE) { color = ImVec4(1, 0.65f, 0, 1); status_text = "[Update]"; }
                
                bool selected = p_state->archive_selection[i] != 0;
                if (ImGui::Checkbox(("##" + archive.name + std::to_string(i)).c_str(), &selected)) {
                    p_state->archive_selection[i] = selected;
                }
                ImGui::SameLine();
                ImGui::TextColored(color, "%s [%s] %s", archive.name.c_str(), archive.version.c_str(), status_text.c_str());
            }

             ImGui::EndChild();
             ImGui::EndTabItem();
        }

        // --- TAB 1: MOD CONFIGURATION ---
        if (ImGui::BeginTabItem("Mod Configuration")) {
            ImGui::BeginChild("ModTree", ImVec2(0, -50), true);
            
            for (auto& mod : p_state->mod_manager.mod_definitions) {
                // Mod name itself is unique, so this is fine.
                if (ImGui::Checkbox(mod.name.c_str(), &mod.enabled)) {
                    state_changed = true;
                    if (mod.enabled) {
                        for (auto& group : mod.option_groups) {
                            if (group.required && !group.options.empty()) group.options.front().enabled = true;
                        }
                    }
                }

                if (mod.enabled) {
                    ImGui::Indent();
                    for (auto& group : mod.option_groups) {
                        ImGui::TextDisabled("  - %s -", group.name.c_str());
                        ImGui::Indent();
                        for (auto& option : group.options) {
                            // --- THIS IS THE FIX ---
                            // Create a unique ID by combining the option, group, and mod names.
                            // The part after ## is used for the ID but is not displayed.
                            std::string unique_id = option.name + "##" + mod.name + group.name;
                            if (ImGui::Checkbox(unique_id.c_str(), &option.enabled)) {
                                handle_option_check(option);
                                state_changed = true;
                            }
                        }
                        ImGui::Unindent();
                    }
                    ImGui::Unindent();
                }
                ImGui::Separator();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // --- TAB 2: DATA PATH ORDER ---
        if (ImGui::BeginTabItem("Data Path Order")) {
            ImGui::Text("D-Pad to navigate, L1/R1 to reorder.");
            ImGui::SameLine();
            if (p_state->rules_loaded) {
                if (ImGui::Button("[Auto-Sort]")) {
                    p_state->sort_manager.sort_data_paths(p_state->mod_manager.active_data_paths, ctx.path_mod_data);
                }
            }
            ImGui::Separator();
            ImGui::BeginChild("DataList", ImVec2(0, -50), true);
            
            p_state->focused_data_idx = -1; // Default to no focus
            for (size_t i = 0; i < p_state->mod_manager.active_data_paths.size(); ++i) {
                if (p_state->focus_request_data && p_state->focused_data_idx == (int)i) {
                    ImGui::SetItemDefaultFocus();
                    p_state->focus_request_data = false;
                }
                std::string label = p_state->mod_manager.active_data_paths[i].lexically_relative(ctx.path_mod_data).string();
                if(ImGui::Selectable(label.c_str(), p_state->focused_data_idx == (int)i)) { /* Click selection */ }
                if (ImGui::IsItemFocused()) { p_state->focused_data_idx = i; }
            }

            // --- Reordering logic is now INSIDE the tab's scope ---
            if (p_state->focused_data_idx != -1) {
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && p_state->focused_data_idx > 0) {
                    std::swap(p_state->mod_manager.active_data_paths[p_state->focused_data_idx], p_state->mod_manager.active_data_paths[p_state->focused_data_idx - 1]);
                    p_state->focused_data_idx--;
                    p_state->focus_request_data = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) && p_state->focused_data_idx < p_state->mod_manager.active_data_paths.size() - 1) {
                    std::swap(p_state->mod_manager.active_data_paths[p_state->focused_data_idx], p_state->mod_manager.active_data_paths[p_state->focused_data_idx + 1]);
                    p_state->focused_data_idx++;
                    p_state->focus_request_data = true;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // --- TAB 3: CONTENT FILE ORDER ---
        if (ImGui::BeginTabItem("Content File Order")) {
            ImGui::Text("A to toggle, D-Pad to navigate, L1/R1 to reorder.");
            ImGui::SameLine();
            if (p_state->rules_loaded) {
                if (ImGui::Button("[Auto-Sort]")) {
                    p_state->sort_manager.sort_content_files(p_state->mod_manager.active_content_files);
                }
            }
            ImGui::Separator();
            ImGui::BeginChild("ContentList", ImVec2(0, -50), true);

            p_state->focused_content_idx = -1; // Default to no focus
            for (size_t i = 0; i < p_state->mod_manager.active_content_files.size(); ++i) {
                auto& content = p_state->mod_manager.active_content_files[i];
                if (p_state->focus_request_content && p_state->focused_content_idx == (int)i) {
                    ImGui::SetItemDefaultFocus();
                    p_state->focus_request_content = false;
                }
                if (ImGui::Checkbox(content.name.c_str(), &content.enabled)) { state_changed = true; }
                if (ImGui::IsItemFocused()) { p_state->focused_content_idx = i; }
            }
            
            if (p_state->focused_content_idx != -1) {
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && p_state->focused_content_idx > 0) {
                    std::swap(p_state->mod_manager.active_content_files[p_state->focused_content_idx], p_state->mod_manager.active_content_files[p_state->focused_content_idx - 1]);
                    p_state->focused_content_idx--;
                    p_state->focus_request_content = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) && p_state->focused_content_idx < p_state->mod_manager.active_content_files.size() - 1) {
                    std::swap(p_state->mod_manager.active_content_files[p_state->focused_content_idx], p_state->mod_manager.active_content_files[p_state->focused_content_idx + 1]);
                    p_state->focused_content_idx++;
                    p_state->focus_request_content = true;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    if (state_changed) { p_state->mod_manager.update_active_lists(); }

    if (ImGui::Button("Save and Exit", ImVec2(150, 40))) {
        // p_state->mod_manager.update_active_lists();
        fs::path config_path = ctx.path_openmw_cfg;

        p_state->config_manager.data_paths.clear();
        for (const auto& path : p_state->mod_manager.active_data_paths) p_state->config_manager.data_paths.push_back(path.string());
        
        // FIX #2: Manually convert vector<ContentFile> to vector<string>
        p_state->config_manager.content_files.clear();
        for (const auto& content_file : p_state->mod_manager.active_content_files) {
            if (content_file.enabled) {
                p_state->config_manager.content_files.push_back(content_file.name);
            }
        }
        
        p_state->config_manager.save(config_path);
        next_scene = std::make_unique<MainMenuScene>();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(150, 40))) {
        next_scene = std::make_unique<MainMenuScene>();
    }


    ImGui::End();
}
