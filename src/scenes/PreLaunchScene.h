#pragma once
#include "Scene.h"
#include "../mod/ScriptManager.h"
#include <memory>

class ScriptRunner;
class ScriptRunnerScene;

class PreLaunchScene : public Scene {
public:
    PreLaunchScene(StateMachine& machine, std::vector<ScriptDefinition*> scripts_to_run);

    void on_enter() override;
    void update() override;
    void render() override;
    void handle_event(SDL_Event& e) override;

private:
    void advance_to_next_script();

    enum class State { IDLE, RUNNING_HEADLESS, AWAITING_UI_FINISH, FINISHED };
    State m_state = State::IDLE;

    std::vector<ScriptDefinition*> m_scripts;
    size_t m_current_script_index = 0;
    std::shared_ptr<ScriptRunner> m_current_runner; 

    size_t m_total_enabled_scripts = 0;
    size_t m_completed_scripts = 0;
};
