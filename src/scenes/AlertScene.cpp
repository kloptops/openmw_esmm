#include "AlertScene.h"
#include "../core/StateMachine.h"
#include "imgui.h"

AlertScene::AlertScene(StateMachine& machine, const std::string& title, const std::string& message)
    : Scene(machine), m_title(title), m_message(message) {}

void AlertScene::render() {
    // Dim the background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("DimBG", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
    ImGui::End();

    // The modal popup
    ImGui::OpenPopup(m_title.c_str());
    if (ImGui::BeginPopupModal(m_title.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_message.c_str());
        ImGui::Separator();

        // Center the button
        float button_width = 120;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - button_width) * 0.5f);
        if (ImGui::Button("OK", ImVec2(button_width, 0))) {
            ImGui::CloseCurrentPopup();
            m_state_machine.pop_state(); // This is the key to unblocking the runner
        }
        ImGui::EndPopup();
    }
}
