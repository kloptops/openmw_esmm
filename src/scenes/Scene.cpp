#include "Scene.h"
#include "../AppContext.h"
#include "imgui.h"

// Define the global scene pointer
std::unique_ptr<Scene> next_scene = nullptr;

MenuScene::MenuScene(const std::string& t) : title(t) {}

void MenuScene::handle_event(SDL_Event& e, AppContext& ctx)
{
    // This input handling logic remains exactly the same!
    // It's still needed for keyboard/controller navigation.
    if (e.type == SDL_KEYDOWN)
    {
        switch (e.key.keysym.sym)
        {
        case SDLK_UP:
            selected_index = (selected_index - 1 + options.size()) % options.size();
            break;
        case SDLK_DOWN:
            selected_index = (selected_index + 1) % options.size();
            break;
        case SDLK_RETURN:
        case SDLK_SPACE:
            on_select(selected_index, ctx);
            break;
        }
    }
    else if (e.type == SDL_CONTROLLERBUTTONDOWN)
    {
        switch(e.cbutton.button)
        {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            selected_index = (selected_index - 1 + options.size()) % options.size();
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            selected_index = (selected_index + 1) % options.size();
            break;
        case SDL_CONTROLLER_BUTTON_A:
            on_select(selected_index, ctx);
            break;
        case SDL_CONTROLLER_BUTTON_B:
            on_back(ctx);
            break;
        }
    }
}

// --- THIS IS THE NEW RENDER FUNCTION ---
void MenuScene::render(AppContext& ctx)
{
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
    for (size_t i = 0; i < options.size(); ++i)
    {
        // Center each menu item
        textWidth = ImGui::CalcTextSize(options[i].c_str()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        
        // The selectable item
        if (ImGui::Selectable(options[i].c_str(), selected_index == (int)i))
        {
            // We can also trigger selection by clicking now
            on_select(i, ctx);
        }
    }

    ImGui::End();
}
