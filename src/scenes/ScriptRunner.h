#pragma once
#include <csignal>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../mod/ScriptManager.h" // For ScriptDefinition
#include "../core/ModEngine.h"

// Forward declare StateMachine to break circular dependency
class StateMachine;

struct ProgressState {
    std::string name;
    std::string message;
    int step = 0;
    int total = 100;
};

struct AlertInfo {
    std::string title;
    std::string message;
};

struct ScriptRunResult {
    int return_code = -1;
    std::string output;
    std::unique_ptr<fs::path> modified_cfg_path;
};

class ScriptRunner {
public:
    ScriptRunner(StateMachine& machine, ScriptDefinition& script);
    virtual ~ScriptRunner() = default;

    // The main execution entry point
    virtual void run(const std::map<std::string, std::string>& extra_vars = {}, bool use_temp_cfg = false);

    void request_cancellation();

    pid_t get_pid() const { return m_pid; }

    bool is_finished() const { return m_is_finished; }
    const ScriptRunResult& get_result() const { return m_result; } 
    ScriptDefinition& get_script() { return m_script; } // Needed for a clean implementation later

protected:
    // Overridden by subclasses to handle output and state changes
    virtual void on_line_received(const std::string& line);
    virtual void on_progress_update();
    virtual void on_alert(const AlertInfo& alert);
    virtual void on_finish(int return_code);

    void parse_line(const std::string& line);

    StateMachine& m_state_machine;
    ScriptDefinition& m_script;
    ProgressState m_progress;

    pid_t m_pid = -1;

    fs::path m_cancel_file_path;

    bool m_is_finished = false;
    ScriptRunResult m_result;
};

class HeadlessScriptRunner : public ScriptRunner {
public:
    using ScriptRunner::ScriptRunner;
    bool alert_was_triggered() const { return m_alert_triggered; }

protected:
    void on_alert(const AlertInfo& alert) override;

private:
    bool m_alert_triggered = false;
};

// Forward declare to link runner and scene
class ScriptRunnerScene;

class UIScriptRunner : public ScriptRunner {
public:
    UIScriptRunner(StateMachine& machine, ScriptDefinition& script);

    void set_scene_ptr(ScriptRunnerScene* scene_ptr);

    void request_cancellation() { ScriptRunner::request_cancellation(); }

private:
    friend class ScriptRunnerScene;

    // Overrides to update the UI scene
    void on_line_received(const std::string& line) override;
    void on_progress_update() override;
    void on_alert(const AlertInfo& alert) override;
    void on_finish(int return_code) override;
    
    ScriptRunnerScene* m_scene_ptr = nullptr;
};

class PreLaunchScriptRunner {
public:
    PreLaunchScriptRunner(StateMachine& machine, const std::vector<ScriptDefinition*>& scripts);

    // Runs all scripts sequentially and returns true if the game should launch
    bool run();

private:
    StateMachine& m_state_machine;
    std::vector<ScriptDefinition*> m_scripts;
};
