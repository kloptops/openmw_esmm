#include "UtilitiesScene.h"
#include "ScriptRunner.h"
#include "ScriptRunnerScene.h"
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
    float top_bar_height = 50.0f;

    auto pre_launch_scripts = script_manager.get_scripts_by_registration(ScriptRegistration::RUN_BEFORE_LAUNCH);
    if (!pre_launch_scripts.empty()) {
        ImGui::Text("Scripts to Run Before Launch:");
        std::string run_order;
        for (auto* script : pre_launch_scripts) {
            if (script->enabled) {
                if (!run_order.empty()) run_order += " -> ";
                run_order += script->title;
            }
        }
        if (run_order.empty()) {
            ImGui::TextDisabled("(None are enabled)");
        } else {
            ImGui::TextWrapped("%s", run_order.c_str());
        }
        ImGui::Separator();
        top_bar_height = ImGui::GetCursorPosY() + 5.0f;
    }

    float list_height = ImGui::GetWindowSize().y - top_bar_height - bottom_bar_height;
    ImGui::BeginChild("ScriptList", ImVec2(0, list_height), true);

    auto& all_scripts = script_manager.get_all_scripts_mut();
    auto data_sorters = script_manager.get_scripts_by_registration(ScriptRegistration::SORT_DATA);
    auto content_sorters = script_manager.get_scripts_by_registration(ScriptRegistration::SORT_CONTENT);
    auto content_verifiers = script_manager.get_scripts_by_registration(ScriptRegistration::VERIFY);

    if (all_scripts.empty()) {
        ImGui::TextWrapped("No scripts found in the 'scripts' directory.");
    }
    
    ImGui::Text("Available Scripts");
    ImGui::Separator();
    
    for (auto& script : all_scripts) {
        const std::string id = "##" + script.script_path.string();

        if (ImGui::Checkbox((script.title + id).c_str(), &script.enabled)) {
            script_manager.save_options(options_path);
        }

        ImGui::BeginDisabled(!script.enabled);
        
        ImGui::SameLine();
        ImGui::TextDisabled("- %s", script.description.c_str());

        ImGui::Indent();
        if (ImGui::Button(("Run" + id).c_str())) {
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

        if (!script.config_options.empty()) {
            ImGui::SameLine();
            if (ImGui::Button(("Configure" + id).c_str())) {
                p_state->script_to_configure = &script;
                p_state->temp_options = script.config_options;
                p_state->open_config_popup = true;
            }
        }
        ImGui::Unindent();
        ImGui::EndDisabled();
        ImGui::Separator();
    }

    // --- SORTER SELECTION UI ---
    if (!data_sorters.empty()) {
        ImGui::Dummy(ImVec2(0, 20.0f));
        ImGui::Text("Active Data Sorter");
        ImGui::Separator();
        for (auto* script : data_sorters) {
            const std::string id = "##_data_sort_" + script->script_path.string();
            if (ImGui::RadioButton((script->title + id).c_str(), script_manager.get_active_data_sorter_path() == script->script_path)) {
                script_manager.set_active_data_sorter_path(script->script_path);
                script_manager.save_options(options_path);
            }
        }
    }

    if (!content_sorters.empty()) {
        ImGui::Dummy(ImVec2(0, 20.0f));
        ImGui::Text("Active Content Sorter");
        ImGui::Separator();
        for (auto* script : content_sorters) {
            const std::string id = "##_content_sort_" + script->script_path.string();
            if (ImGui::RadioButton((script->title + id).c_str(), script_manager.get_active_content_sorter_path() == script->script_path)) {
                script_manager.set_active_content_sorter_path(script->script_path);
                script_manager.save_options(options_path);
            }
        }
    }

    if (!content_verifiers.empty()) {
        ImGui::Dummy(ImVec2(0, 20.0f));
        ImGui::Text("Active Content Verifier");
        ImGui::Separator();
        for (auto* script : content_verifiers) {
            const std::string id = "##_content_verifier_" + script->script_path.string();
            if (ImGui::RadioButton((script->title + id).c_str(), script_manager.get_active_content_verifier_path() == script->script_path)) {
                script_manager.set_active_content_verifier_path(script->script_path);
                script_manager.save_options(options_path);
            }
        }
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
