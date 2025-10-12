#include "ModManagerScene.h"
#include "ExtractorScene.h"
#include "../core/StateMachine.h"
#include "../core/ModEngine.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <set>

void handle_option_check(ModOption& option) {
    if (!option.enabled) return;
    if (option.parent_group && option.parent_group->type == ModOptionGroup::SINGLE_CHOICE) {
        for (auto& other_option : option.parent_group->options) {
            if (&other_option != &option) other_option.enabled = false;
        }
    }
}

// --- NEW DISPLAY HELPER FUNCTION ---
static std::string get_display_path(const fs::path& path, const ModManager& mod_manager, const AppContext& ctx) {
    for (const auto& mod : mod_manager.mod_definitions) {
        for (const auto& group : mod.option_groups) {
            for (const auto& option : group.options) {
                if (option.path == path) {
                    // Found the owner mod and option
                    bool is_external = mod.root_path.string().find(ctx.path_mod_data.string()) != 0;
                    
                    if (mod.name == "The Elder Scrolls III: Morrowind") {
                        return mod.name + "/" + option.name;
                    }

                    std::string relative_part = path.lexically_relative(mod.root_path).string();
                    return (is_external ? "[External] " : "") + mod.name + "/" + relative_part;
                }
            }
        }
    }
    // Fallback if no owner is found
    return path.string();
}

// The State is now much simpler, only holding UI state
enum class ActiveTab { ARCHIVES, CONFIG, CONTENT_ORDER, DATA_ORDER, VALIDATION };
struct ModManagerScene::State {
    ActiveTab current_tab = ActiveTab::ARCHIVES;
    int focused_data_idx = -1;
    int focused_content_idx = -1;
    bool focus_request_data = false;
    bool focus_request_content = false;
    std::vector<char> archive_selection;
    bool show_save_warning = false;
    bool needs_refresh = true;
};

ModManagerScene::ModManagerScene(StateMachine& machine) : Scene(machine) {
    p_state = new State();
}
ModManagerScene::~ModManagerScene() { delete p_state; }

void ModManagerScene::save_and_exit() {
    m_state_machine.get_engine().save_configuration();
    m_state_machine.pop_state(); // Pop back to main menu
}

void ModManagerScene::fix_load_order_and_save() {
    ModManager& mod_manager = m_state_machine.get_engine().get_mod_manager_mut();

    // Fix data paths
    auto& data_paths = mod_manager.active_data_paths;
    auto it_data = std::find_if(data_paths.begin(), data_paths.end(), [](const fs::path& p) {
        return fs::exists(p / "Morrowind.esm");
    });
    if (it_data != data_paths.end() && it_data != data_paths.begin()) {
        fs::path game_data_path = *it_data;
        data_paths.erase(it_data);
        data_paths.insert(data_paths.begin(), game_data_path);
    }

    // Fix content files
    auto& content_files = mod_manager.active_content_files;
    const std::vector<std::string> masters = {"Morrowind.esm", "Tribunal.esm", "Bloodmoon.esm"};

    // Iterate forwards to find, then backwards to insert
    for (int i = masters.size() - 1; i >= 0; --i) {
        auto it_content = std::find_if(content_files.begin(), content_files.end(), [&](const ContentFile& cf){
            return cf.name == masters[i];
        });
        if (it_content != content_files.end()) {
            ContentFile master_file = *it_content;
            content_files.erase(it_content);
            master_file.enabled = true; // <-- THE FIX: Force it to be enabled
            content_files.insert(content_files.begin(), master_file);
        }
    }
    
    save_and_exit();
}

void ModManagerScene::on_enter() {
    p_state->needs_refresh = true;
}

// handle_event is empty because all logic is in render
void ModManagerScene::handle_event(SDL_Event& e) {}

void ModManagerScene::render() {
    // Get references to the engine and its managers
    ModEngine& engine = m_state_machine.get_engine();
    ModManager& mod_manager = engine.get_mod_manager_mut();
    const ArchiveManager& archive_manager = engine.get_archive_manager();
    const AppContext& ctx = m_state_machine.get_context();

    if (p_state->needs_refresh) {
        engine.rescan_archives();
        engine.rescan_mods();
        p_state->archive_selection.assign(engine.get_archive_manager().archives.size(), 0);
        p_state->needs_refresh = false;
    }

    bool state_changed = false;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Mod Manager", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Archive Management")) {

            bool has_new_mods = false;
            bool has_updated_mods = false;
            for (const auto& archive : engine.get_archive_manager().archives) {
                if (archive.status == ArchiveInfo::NEW) has_new_mods = true;
                if (archive.status == ArchiveInfo::UPDATE_AVAILABLE) has_updated_mods = true;
            }

            if (ImGui::Button("Extract/Update Selected")) {
                std::vector<ArchiveInfo> to_extract;
                for(size_t i = 0; i < archive_manager.archives.size(); ++i) {
                    if (p_state->archive_selection[i]) to_extract.push_back(archive_manager.archives[i]);
                }
                if (!to_extract.empty()) {
                    p_state->needs_refresh = true;
                    m_state_machine.push_scene(std::make_unique<ExtractorScene>(m_state_machine, to_extract));
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Delete Selected Mod Data")) {
                std::vector<fs::path> paths_to_delete;
                for(size_t i = 0; i < engine.get_archive_manager().archives.size(); ++i) {
                    if (p_state->archive_selection[i]) {
                        const auto& archive = engine.get_archive_manager().archives[i];
                        if (fs::exists(archive.target_data_path)) {
                            paths_to_delete.push_back(archive.target_data_path);
                        }
                    }
                }

                if (!paths_to_delete.empty()) {
                    engine.delete_mod_data(paths_to_delete);
                    // --- Immediately trigger a refresh ---
                    p_state->needs_refresh = true; 
                }
            }
            // --- NEW: Color-coded toggle buttons ---
            if (has_new_mods) {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.33f, 0.8f, 0.8f)); // Green
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.33f, 0.9f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.33f, 1.0f, 1.0f));
                if (ImGui::Button("Toggle New")) {
                    for (size_t i = 0; i < engine.get_archive_manager().archives.size(); ++i) {
                        if (engine.get_archive_manager().archives[i].status == ArchiveInfo::NEW) {
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
                    for (size_t i = 0; i < engine.get_archive_manager().archives.size(); ++i) {
                        if (engine.get_archive_manager().archives[i].status == ArchiveInfo::UPDATE_AVAILABLE) {
                            p_state->archive_selection[i] = !p_state->archive_selection[i];
                        }
                    }
                }
                ImGui::PopStyleColor(3);
            }

            ImGui::Separator();

             
            ImGui::BeginChild("ArchiveList", ImVec2(0, -50), true);
            for (size_t i = 0; i < engine.get_archive_manager().archives.size(); ++i) {
                auto& archive = engine.get_archive_manager().archives[i];
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
            
            for (auto& mod : mod_manager.mod_definitions) {
                // --- THIS IS THE FIX ---
                // Identify if this is the special, non-disable-able base game mod.
                bool is_base_game_mod = (mod.name == "The Elder Scrolls III: Morrowind");

                if (is_base_game_mod) {
                    ImGui::BeginDisabled(); // Disable the widget
                }

                if (ImGui::Checkbox(mod.name.c_str(), &mod.enabled)) {
                    state_changed = true;
                    if (mod.enabled) {
                        for (auto& group : mod.option_groups) {
                            if (group.required && !group.options.empty()) group.options.front().enabled = true;
                        }
                    }
                }

                if (is_base_game_mod) {
                    mod.enabled = true; // Forcibly re-enable it just in case.
                    ImGui::EndDisabled(); // Re-enable widgets for the next item
                }

                if (mod.enabled) {
                    ImGui::Indent();
                    for (auto& group : mod.option_groups) {
                        ImGui::TextDisabled("  - %s -", group.name.c_str());
                        ImGui::Indent();
                        for (auto& option : group.options) {
                            // --- THIS IS THE SECOND PART OF THE FIX ---
                            // Check if this specific option is the base game's data path.
                            bool is_base_data_option = is_base_game_mod && fs::exists(option.path / "Morrowind.esm");

                            if (is_base_data_option) {
                                ImGui::BeginDisabled();
                            }

                            std::string unique_id = option.name + "##" + mod.name + group.name;
                            if (ImGui::Checkbox(unique_id.c_str(), &option.enabled)) {
                                handle_option_check(option);
                                state_changed = true;
                            }

                            if (is_base_data_option) {
                                option.enabled = true; // And forcibly re-enable it.
                                ImGui::EndDisabled();
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

        // --- TAB 2: CONTENT FILE ORDER ---
        if (ImGui::BeginTabItem("Content File Order")) {
            ImGui::Text("A to toggle, D-Pad to navigate, L1/R1 to reorder.");
    
            if (engine.has_active_sorter(ScriptRegistration::SORT_CONTENT)) {
                ImGui::SameLine();
                if (ImGui::Button("Sort Content")) {
                    engine.run_active_sorter(ScriptRegistration::SORT_CONTENT);
                    p_state->needs_refresh = true;
                }
            }

            if (engine.has_active_verifier()) {
                ImGui::SameLine();
                if (ImGui::Button("Verify Content")) {
                    engine.run_active_verifier();
                }
            }

            ImGui::Separator();
            ImGui::BeginChild("ContentList", ImVec2(0, -50), true);

            p_state->focused_content_idx = -1; // Default to no focus
            for (size_t i = 0; i < mod_manager.active_content_files.size(); ++i) {
                auto& content = mod_manager.active_content_files[i];
                bool content_is_new = false;

                if (content.is_new) {
                    // Push a yellow color for the text of new plugins
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
                    content_is_new = true;
                }

                if (p_state->focus_request_content && p_state->focused_content_idx == (int)i) {
                    ImGui::SetItemDefaultFocus();
                    p_state->focus_request_content = false;
                }
                if (ImGui::Checkbox(content.name.c_str(), &content.enabled)) {
                    // If the user interacts with it, it's no longer "new"
                    content.is_new = false;
                }
                if (ImGui::IsItemFocused()) { p_state->focused_content_idx = i; }

                if (content_is_new) {
                    ImGui::PopStyleColor(); // Don't forget to pop the color!
                }
            }
            
            if (p_state->focused_content_idx != -1) {
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && p_state->focused_content_idx > 0) {
                    std::swap(mod_manager.active_content_files[p_state->focused_content_idx], mod_manager.active_content_files[p_state->focused_content_idx - 1]);
                    p_state->focused_content_idx--;
                    p_state->focus_request_content = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) && p_state->focused_content_idx < mod_manager.active_content_files.size() - 1) {
                    std::swap(mod_manager.active_content_files[p_state->focused_content_idx], mod_manager.active_content_files[p_state->focused_content_idx + 1]);
                    p_state->focused_content_idx++;
                    p_state->focus_request_content = true;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // --- TAB 3: DATA PATH ORDER ---
        if (ImGui::BeginTabItem("Data Path Order")) {
            ImGui::Text("D-Pad to navigate, L1/R1 to reorder.");

            if (engine.has_active_sorter(ScriptRegistration::SORT_DATA)) {
                ImGui::SameLine();
                if (ImGui::Button("Sort Data")) {
                    engine.run_active_sorter(ScriptRegistration::SORT_DATA);
                    p_state->needs_refresh = true;
                }
            }

            ImGui::Separator();
            ImGui::BeginChild("DataList", ImVec2(0, -50), true);
            
            p_state->focused_data_idx = -1; // Default to no focus
            for (size_t i = 0; i < mod_manager.active_data_paths.size(); ++i) {
                if (p_state->focus_request_data && p_state->focused_data_idx == (int)i) {
                    ImGui::SetItemDefaultFocus();
                    p_state->focus_request_data = false;
                }
                
                // --- THIS IS THE FIX ---
                std::string label = get_display_path(mod_manager.active_data_paths[i], mod_manager, ctx);

                if (ImGui::Selectable(label.c_str(), p_state->focused_data_idx == (int)i)) { p_state->focused_data_idx = i; }
                if (ImGui::IsItemFocused()) { p_state->focused_data_idx = i; }
            }

            // --- Reordering logic is now INSIDE the tab's scope ---
            if (p_state->focused_data_idx != -1) {
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadL1, false) && p_state->focused_data_idx > 0) {
                    std::swap(mod_manager.active_data_paths[p_state->focused_data_idx], mod_manager.active_data_paths[p_state->focused_data_idx - 1]);
                    p_state->focused_data_idx--;
                    p_state->focus_request_data = true;
                }
                if (ImGui::IsKeyPressed(ImGuiKey_GamepadR1, false) && p_state->focused_data_idx < mod_manager.active_data_paths.size() - 1) {
                    std::swap(mod_manager.active_data_paths[p_state->focused_data_idx], mod_manager.active_data_paths[p_state->focused_data_idx + 1]);
                    p_state->focused_data_idx++;
                    p_state->focus_request_data = true;
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        // --- TAB 4: VALIDATION ---
        if (ImGui::BeginTabItem("Validation")) {

            ImGui::Text("Validation report coming soon!");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    if (state_changed) {
        mod_manager.update_active_lists();
    }

    if (ImGui::Button("Save and Exit", ImVec2(150, 40))) {
        // --- NEW VALIDATION LOGIC ---
        const auto& data = mod_manager.active_data_paths;
        const auto& content = mod_manager.active_content_files;
        
        bool data_ok = !data.empty() && fs::exists(data.front() / "Morrowind.esm");
        // Check for presence, order, AND enabled status
        bool content_ok = content.size() >= 3 &&
                          content[0].name == "Morrowind.esm" && content[0].enabled &&
                          content[1].name == "Tribunal.esm" && content[1].enabled &&
                          content[2].name == "Bloodmoon.esm" && content[2].enabled;

        if (data_ok && content_ok) {
            save_and_exit();
        } else {
            p_state->show_save_warning = true;
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(150, 40))) {
        m_state_machine.pop_state(); // Pop back to main menu
    }

    if (p_state->show_save_warning) {
        ImGui::OpenPopup("Save Warning");
        p_state->show_save_warning = false; // Reset trigger
    }

    if (ImGui::BeginPopupModal("Save Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Your load order has critical issues!\n\n"
                    "- Morrowind.esm must be in the first data path.\n"
                    "- Morrowind.esm, Tribunal.esm, and Bloodmoon.esm\n"
                    "  must be the first three ENABLED content files, in order.\n\n"
                    "Saving this configuration may cause OpenMW to fail to launch.\n");
        ImGui::Separator();

        if (ImGui::Button("Fix it for me", ImVec2(150, 0))) {
            fix_load_order_and_save();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Anyway", ImVec2(150, 0))) {
            save_and_exit();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
