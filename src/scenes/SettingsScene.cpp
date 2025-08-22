#include "SettingsScene.h"
#include "../core/StateMachine.h"
#include "imgui.h"

SettingsScene::SettingsScene(StateMachine& machine) : Scene(machine) {
    m_options = {"(Nothing to configure yet)", "Back to Main Menu"};
}

void SettingsScene::handle_event(SDL_Event& e) {
    // if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_UP) || (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP)) {
    //     m_selected_index = (m_selected_index - 1 + m_options.size()) % m_options.size();
    // }
    // if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_DOWN) || (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)) {
    //     m_selected_index = (m_selected_index + 1) % m_options.size();
    // }
    // if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) || (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_A)) {
    //     on_select();
    // }
    // // Allow 'B' button to go back
    // if (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
    //     m_state_machine.pop_state();
    // }
}

void SettingsScene::on_select() {
    // Both options currently just go back
    m_state_machine.pop_state();
}

void SettingsScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    
    // Simple menu rendering
    for (size_t i = 0; i < m_options.size(); ++i) {
        if (ImGui::Selectable(m_options[i].c_str(), m_selected_index == (int)i)) {
            m_selected_index = i;
            on_select();
        }
    }
    
    ImGui::End();
}
