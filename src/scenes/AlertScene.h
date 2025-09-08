#pragma once
#include "Scene.h"
#include <string>

class AlertScene : public Scene {
public:
    AlertScene(StateMachine& machine, const std::string& title, const std::string& message);

    void render() override;

private:
    std::string m_title;
    std::string m_message;
};
