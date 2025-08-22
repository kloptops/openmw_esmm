#pragma once
#include <vector>
#include <memory>
#include "../scenes/Scene.h"
#include "ModEngine.h"

// An enum to identify each state without needing class names
enum class StateID {
    MainMenu,
    ModManager,
    Settings,
    Extractor
};

class StateMachine {
public:
    StateMachine(AppContext& ctx);
    ~StateMachine() = default;

    void handle_event(SDL_Event& e);
    void update();
    void render();

    void push_state(StateID id);
    void pop_state();
    void change_state(StateID id);

    bool is_running() const { return !m_states.empty(); }
    ModEngine& get_engine() { return m_engine; }
    AppContext& get_context() { return m_context; }
    
    // Special function for scenes with complex constructors
    void push_extractor_scene(const std::vector<ArchiveInfo>& archives);

private:
    std::unique_ptr<Scene> create_scene(StateID id);
    void process_state_changes();

    AppContext& m_context;
    ModEngine m_engine;

    std::vector<std::unique_ptr<Scene>> m_states;

    struct PendingChange {
        enum Action { Push, Pop, Change };
        Action action;
        StateID id;
    };
    std::vector<PendingChange> m_pending_changes;
    std::unique_ptr<Scene> m_extractor_scene_to_push = nullptr;
};
