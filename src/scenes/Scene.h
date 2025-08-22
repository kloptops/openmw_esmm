#pragma once
#include <SDL2/SDL.h>
#include <memory>

class StateMachine;

class Scene {
public:
    Scene(StateMachine& machine); // All scenes get the state machine
    virtual ~Scene() = default;

    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void handle_event(SDL_Event& e) {}
    virtual void update() {}
    virtual void render() {}

protected:
    StateMachine& m_state_machine;
};
