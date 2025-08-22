#include "MainMenuScene.h"
#include "../core/StateMachine.h" // The key include
#include "../AppContext.h"
#include "imgui.h"

MainMenuScene::MainMenuScene(StateMachine& machine) : Scene(machine) {
    m_options = {"Load Morrowind", "Mod Manager", "Settings", "Quit"};
}

void MainMenuScene::handle_event(SDL_Event& e) {
    // if (e.type == SDL_KEYDOWN) {
    //     if (e.key.keysym.sym == SDLK_UP) m_selected_index = (m_selected_index - 1 + m_options.size()) % m_options.size();
    //     if (e.key.keysym.sym == SDLK_DOWN) m_selected_index = (m_selected_index + 1) % m_options.size();
    //     if (e.key.keysym.sym == SDLK_RETURN) on_select();
    // } else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
    //     if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) m_selected_index = (m_selected_index - 1 + m_options.size()) % m_options.size();
    //     if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) m_selected_index = (m_selected_index + 1) % m_options.size();
    //     if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) on_select();
    // }
}

void MainMenuScene::on_select(int i) {
    switch (i) {
        case 0:
            m_state_machine.get_context().running = false;
            m_state_machine.get_context().exit_code = 0;
            break;
        case 1:
            m_state_machine.push_state(StateID::ModManager);
            break;
        case 2:
            m_state_machine.push_state(StateID::Settings);
            break;
        case 3:
            m_state_machine.get_context().running = false;
            m_state_machine.get_context().exit_code = 255;
            break;
    }
}

void MainMenuScene::render() {
    std::string title = "OpenMW ESMM";
    // Create a full-screen, un-decorated window to act as our scene background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    // --- Title ---
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 50); // Add some top padding
    // Calculate position to center the title
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(title.c_str()).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", title.c_str());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20); // Add padding after title
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 50.0f)); // More vertical space

    // --- Menu Options ---
    for (size_t i = 0; i < m_options.size(); ++i)
    {
        // Center each menu item
        textWidth = ImGui::CalcTextSize(m_options[i].c_str()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

        // The selectable item
        if (ImGui::Selectable(m_options[i].c_str()))
        {
            // We can also trigger selection by clicking now
            on_select(i);
        }
    }

    ImGui::End();
}
