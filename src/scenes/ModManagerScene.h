#pragma once
#include "Scene.h"

class ModManagerScene : public Scene {
public:
    ModManagerScene(StateMachine& machine);
    ~ModManagerScene();
    void on_enter() override;
    void handle_event(SDL_Event& e) override;
    void render() override;

private:
    void fix_load_order_and_save();
    void save_and_exit();
    
    struct State;
    State* p_state;
};
