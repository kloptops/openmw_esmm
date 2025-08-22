#pragma once
#include "Scene.h"
#include <string>
#include <vector>

class MainMenuScene : public Scene {
public:
    MainMenuScene(StateMachine& machine);
    void handle_event(SDL_Event& e) override;
    void render() override;
private:
    void on_select(int i);
    std::vector<std::string> m_options;
};
