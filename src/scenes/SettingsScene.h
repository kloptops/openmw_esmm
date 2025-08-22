#pragma once
#include "Scene.h"
#include <string>
#include <vector>

class SettingsScene : public Scene {
public:
    SettingsScene(StateMachine& machine);
    void handle_event(SDL_Event& e) override;
    void render() override;
private:
    void on_select();
    std::vector<std::string> m_options;
    int m_selected_index = 0;
};
