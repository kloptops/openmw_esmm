#pragma once
#include "Scene.h"
#include "ScriptRunner.h"
#include <mutex>

// The scene is now a friend of the runner that controls it
class UIScriptRunner;

class ScriptRunnerScene : public Scene {
public:
    ScriptRunnerScene(StateMachine& machine, std::shared_ptr<UIScriptRunner> owner, ScriptDefinition& script, bool use_temp_cfg);

    void on_enter() override;

    void render() override;

private:
    friend class UIScriptRunner; // Allow owner to access private members

    std::shared_ptr<UIScriptRunner> m_owner; // Scene OWNS the runner
    ScriptDefinition& m_script;

    std::mutex m_log_mutex;
    std::vector<std::string> m_log_lines;

    std::mutex m_progress_mutex;
    ProgressState m_progress;
    AlertInfo m_alert;

    bool m_use_temp_cfg; // Add member
    bool m_is_finished = false;
    bool m_show_alert = false;
};
