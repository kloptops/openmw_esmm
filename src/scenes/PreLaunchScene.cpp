#include "PreLaunchScene.h"
#include "../core/StateMachine.h"
#include "../utils/Logger.h"
#include "ScriptRunner.h"
#include "ScriptRunnerScene.h"
#include "MainMenuScene.h"
#include "AlertScene.h"
#include "imgui.h"
#include <thread>

PreLaunchScene::PreLaunchScene(StateMachine& machine, std::vector<ScriptDefinition*> scripts)
    : Scene(machine), m_scripts(std::move(scripts)) {}

void PreLaunchScene::on_enter() {
    for (const auto* script : m_scripts) {
        if (script->enabled) {
            m_total_enabled_scripts++;
        }
    }
    advance_to_next_script();
}

void PreLaunchScene::advance_to_next_script() {
    m_current_runner.reset();
    
    while (m_current_script_index < m_scripts.size() && !m_scripts[m_current_script_index]->enabled) {
        m_current_script_index++;
    }

    if (m_current_script_index >= m_scripts.size()) {
        m_state = State::FINISHED;
        return;
    }
    
    ScriptDefinition* script = m_scripts[m_current_script_index];

    if (script->has_output) {
        m_current_runner = std::make_shared<UIScriptRunner>(m_state_machine, *script);
    } else {
        m_current_runner = std::make_shared<HeadlessScriptRunner>(m_state_machine, *script);
    }
    m_state = State::IDLE;
}

void PreLaunchScene::update() {
    if (m_state == State::FINISHED) {
        m_state_machine.get_context().running = false;
        m_state_machine.get_context().exit_code = 0;
        return;
    }

    // This state means a UI runner scene is on top of us. We just wait for it to be popped.
    if (m_state == State::AWAITING_UI_FINISH) {
        if (m_current_runner && m_current_runner->is_finished()) {
            const auto& result = m_current_runner->get_result();
            m_completed_scripts++;
            
            if (result.return_code == 2) {
                 LOG_INFO("Pre-launch sequence cancelled by user.");
                 m_state_machine.change_scene(std::make_unique<MainMenuScene>(m_state_machine));
                 return;
            }
            if (result.return_code != 0) {
                 LOG_ERROR("Pre-launch script failed. Aborting launch.");
                 m_state_machine.change_scene(std::make_unique<MainMenuScene>(m_state_machine));
                 return;
            }

            m_current_script_index++;
            advance_to_next_script();
        }
    }
}

void PreLaunchScene::handle_event(SDL_Event& e) {
    if (m_state != State::IDLE) return;

    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        ScriptDefinition* script = m_scripts[m_current_script_index];
        if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) { // Run
            if (auto ui_runner = std::dynamic_pointer_cast<UIScriptRunner>(m_current_runner)) {
                // The cast already created the shared_ptr we need. Just pass it.
                auto scene = std::make_unique<ScriptRunnerScene>(m_state_machine, ui_runner, *script, false);
                
                m_state_machine.push_scene(std::move(scene));
                m_state = State::AWAITING_UI_FINISH;

            } else if (auto headless_runner = std::dynamic_pointer_cast<HeadlessScriptRunner>(m_current_runner)) {
                // Capture the shared_ptr to keep it alive for the thread
                std::thread([headless_runner](){ headless_runner->run(); }).detach();
                m_state = State::RUNNING_HEADLESS;
            }
        }
        else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_B) { // Skip Once
            m_current_script_index++;
            m_completed_scripts++;
            advance_to_next_script();
        }
        else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_Y) { // Skip & Disable
            script->enabled = false;
            m_state_machine.get_engine().get_script_manager_mut()->save_options(
                m_state_machine.get_context().path_config_dir / "openmw_esmm_options.cfg"
            );
            m_current_script_index++;
            m_completed_scripts++;
            advance_to_next_script();
        }
        else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_X) { // Cancel
            m_state_machine.pop_state(); // Go back to main menu
        }
    }
}

void PreLaunchScene::render() {
    // If a UI script is running, this scene is hidden, so we do nothing
    if (m_state == State::AWAITING_UI_FINISH) return;

    // Check on headless runners
    if (m_state == State::RUNNING_HEADLESS) {
        if (m_current_runner && m_current_runner->is_finished()) {
            update(); // This will process the result and advance the state
        }
    }

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Launching...", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    if (m_state == State::IDLE && m_current_script_index < m_scripts.size()) {
        ScriptDefinition* script = m_scripts[m_current_script_index];
        ImGui::Text("Pre-Launch Script (%zu / %zu)", m_completed_scripts + 1, m_total_enabled_scripts);
        ImGui::Text("%s", script->title.c_str());
        ImGui::Separator();
        
        ImGui::Text("(A) Run");
        ImGui::Text("(B) Skip Once");
        ImGui::Text("(Y) Skip & Disable");
        ImGui::Text("(X) Cancel");

    } else if (m_state == State::RUNNING_HEADLESS) {
        ImGui::Text("Running headless script (%zu / %zu)...", m_completed_scripts + 1, m_total_enabled_scripts);
        if (m_current_runner) ImGui::Text("%s", m_current_runner->get_script().title.c_str());
    } else if (m_state == State::FINISHED) {
        ImGui::Text("All scripts complete. Launching OpenMW...");
    }

    ImGui::End();
}
