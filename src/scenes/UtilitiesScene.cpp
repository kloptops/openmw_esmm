#include "UtilitiesScene.h"
#include "ScriptRunner.h"
#include "ScriptRunnerScene.h"
#include "AlertScene.h"
#include "../core/StateMachine.h"
#include "../core/ModEngine.h"
#include "ScriptRunner.h"
#include "imgui.h"

struct UtilitiesScene::State {
    ScriptDefinition* script_to_configure = nullptr;
    bool open_config_popup = false;
    std::vector<ScriptOption> temp_options;
};

UtilitiesScene::UtilitiesScene(StateMachine& machine) : Scene(machine) {
    p_state = new State();
}

void UtilitiesScene::on_enter() { }

void UtilitiesScene::render() {
    ScriptManager& script_manager = *m_state_machine.get_engine().get_script_manager_mut();
    const auto& options_path = m_state_machine.get_context().path_config_dir / "openmw_esmm_options.cfg";

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Utilities", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float bottom_bar_height = 50.0f;
    ImGui::BeginChild("MainContent", ImVec2(0, -bottom_bar_height), true);

    // --- SECTION 1: PRE-LAUNCH SCRIPTS ---
    ImGui::Text("Run Before Launch");
    ImGui::Separator();
    auto pre_launch_scripts = script_manager.get_scripts_by_registration(ScriptRegistration::RUN_BEFORE_LAUNCH);

    if (pre_launch_scripts.empty()) {
        ImGui::TextDisabled("No scripts registered to run before launch.");
    } else {
        for (auto* script : pre_launch_scripts) {
            if (ImGui::Checkbox((script->title + " [" + std::to_string(script->priority) + "]").c_str(), &script->enabled)) {
                script_manager.save_options(options_path);
            }
        }
    }

    ImGui::Dummy(ImVec2(0, 10.0f));

    ImGui::Text("General Utilities");
    ImGui::Separator();
    auto& all_scripts = script_manager.get_all_scripts_mut();
    for (auto& script : all_scripts) {
        const std::string id = "##" + script.script_path.string();
        bool is_sorter = script.registration == ScriptRegistration::SORT_DATA ||
                         script.registration == ScriptRegistration::SORT_CONTENT;
        bool is_momw_config = m_state_machine.get_context().is_momw_config;

        ImGui::Text("%s", script.title.c_str());
        ImGui::Indent();
        ImGui::BeginDisabled();
        ImGui::TextWrapped("%s", script.description.c_str());
        ImGui::EndDisabled();
        ImGui::Unindent();

        ImGui::Dummy(ImVec2(0, 10.0f));

        if (ImGui::Button(("Run" + id).c_str())) {
            if (is_sorter && is_momw_config) {
                m_state_machine.push_scene(std::make_unique<AlertScene>(
                    m_state_machine,
                    "Feature Disabled",
                    "Sorting scripts are disabled because your openmw.cfg is managed by Modding OpenMW."
                ));
            } else {
                if (script.has_output) {
                    auto runner = std::make_shared<UIScriptRunner>(m_state_machine, script);
                    
                    // Provide the 'use_temp_cfg' argument, which is false for generic scripts.
                    auto scene = std::make_unique<ScriptRunnerScene>(m_state_machine, runner, script, false);

                    m_state_machine.push_scene(std::move(scene));
                } else {
                    HeadlessScriptRunner runner(m_state_machine, script);
                    runner.run(); // Generic headless scripts don't use temp cfg by default
                }
            }
        }

        if (!script.config_options.empty()) {
            ImGui::SameLine();
            if (ImGui::Button(("Configure" + id).c_str())) {
                p_state->script_to_configure = &script;
                p_state->temp_options = script.config_options;
                p_state->open_config_popup = true;
            }
        }

        ImGui::Separator();
    }
    ImGui::Dummy(ImVec2(0, 10.0f));

    // --- SECTION 3: SORTER/VERIFIER SELECTION ---
    ImGui::Text("Sorters & Verifiers");
    ImGui::Separator();
    
    // Data Sorter Dropdown
    auto data_sorters = script_manager.get_scripts_by_registration(ScriptRegistration::SORT_DATA);
    const char* current_ds_label = "None";
    if (!script_manager.get_active_data_sorter_path().empty()) {
        auto* s = script_manager.get_script_by_path(script_manager.get_active_data_sorter_path());
        if (s) current_ds_label = s->title.c_str();
    }
    if (ImGui::BeginCombo("Data Sorter", current_ds_label)) {
        for (auto* script : data_sorters) {
            bool is_selected = script_manager.get_active_data_sorter_path() == script->script_path;
            if (ImGui::Selectable(script->title.c_str(), is_selected)) {
                script_manager.set_active_data_sorter_path(script->script_path);
                script_manager.save_options(options_path);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Content Sorter Dropdown
    auto content_sorters = script_manager.get_scripts_by_registration(ScriptRegistration::SORT_CONTENT);
    const char* current_cs_label = "None";
    if (!script_manager.get_active_content_sorter_path().empty()) {
        auto* s = script_manager.get_script_by_path(script_manager.get_active_content_sorter_path());
        if (s) current_cs_label = s->title.c_str();
    }
    if (ImGui::BeginCombo("Content Sorter", current_cs_label)) {
        for (auto* script : content_sorters) {
            bool is_selected = script_manager.get_active_content_sorter_path() == script->script_path;
            if (ImGui::Selectable(script->title.c_str(), is_selected)) {
                script_manager.set_active_content_sorter_path(script->script_path);
                script_manager.save_options(options_path);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    // Content Verifier Dropdown
    auto content_verifier = script_manager.get_scripts_by_registration(ScriptRegistration::VERIFY);
    const char* current_cv_label = "None";
    if (!script_manager.get_active_content_verifier_path().empty()) {
        auto* s = script_manager.get_script_by_path(script_manager.get_active_content_verifier_path());
        if (s) current_cv_label = s->title.c_str();
    }
    if (ImGui::BeginCombo("Content Verifier", current_cv_label)) {
        for (auto* script : content_verifier) {
            bool is_selected = script_manager.get_active_content_verifier_path() == script->script_path;
            if (ImGui::Selectable(script->title.c_str(), is_selected)) {
                script_manager.set_active_content_verifier_path(script->script_path);
                script_manager.save_options(options_path);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::Button("Back to Main Menu", ImVec2(-1, 40))) {
        m_state_machine.pop_state();
    }
    
    if (p_state->open_config_popup) {
        ImGui::OpenPopup("Configure Script");
        p_state->open_config_popup = false;
    }

    if (ImGui::BeginPopupModal("Configure Script", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (p_state->script_to_configure) {
            ImGui::Text("Options for %s", p_state->script_to_configure->title.c_str());
            ImGui::Separator();
            
            for (auto& opt : p_state->temp_options) {
                if (opt.type == ScriptOptionType::SELECT) {
                    if (ImGui::BeginCombo(opt.name.c_str(), opt.current_value.c_str())) {
                        for (const auto& item : opt.options) {
                            bool is_selected = (opt.current_value == item);
                            if (ImGui::Selectable(item.c_str(), is_selected)) {
                                opt.current_value = item;
                            }
                            if (is_selected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }
                } else if (opt.type == ScriptOptionType::CHECKBOX) {
                    bool val = (opt.current_value == "true");
                    if (ImGui::Checkbox(opt.name.c_str(), &val)) {
                        opt.current_value = val ? "true" : "false";
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Save", ImVec2(120, 0))) {
                p_state->script_to_configure->config_options = p_state->temp_options;
                script_manager.save_options(options_path);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(120, 0))) {
                for(auto& opt : p_state->temp_options) opt.current_value = opt.default_value;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
