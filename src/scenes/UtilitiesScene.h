#pragma once
#include "Scene.h"
#include "../mod/ScriptManager.h" // For ScriptDefinition

class UtilitiesScene : public Scene {
public:
    UtilitiesScene(StateMachine& machine);

    void on_enter() override;
    void render() override;

private:
    struct State;
    State* p_state;
};
