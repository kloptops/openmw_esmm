#pragma once
#include <vector>
#include <memory>
#include "../scenes/Scene.h"
#include "ModEngine.h"


class StateMachine {
public:
    StateMachine(AppContext& ctx);
    ~StateMachine() = default;

    void handle_event(SDL_Event& e);
    void update();
    void render();

    // The ONLY ways to manipulate the stack
    void push_scene(std::unique_ptr<Scene> scene);
    void pop_state();
    void change_scene(std::unique_ptr<Scene> scene);

    bool is_running() const { return !m_states.empty(); }
    size_t get_stack_size() const { return m_states.size(); }
    ModEngine& get_engine() { return m_engine; }
    AppContext& get_context() { return m_context; }
    
private:
    void process_state_changes();

    AppContext& m_context;
    ModEngine m_engine;
    std::vector<std::unique_ptr<Scene>> m_states;

    struct PendingChange {
        enum Action { Push, Pop, Change };
        Action action;
        std::unique_ptr<Scene> scene;
    };
    std::vector<PendingChange> m_pending_changes;
};
