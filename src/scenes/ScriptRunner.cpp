#include "ScriptRunner.h"
#include "ScriptRunnerScene.h"
#include "AlertScene.h"
#include "../utils/Logger.h"
#include "../core/StateMachine.h"
#include "imgui.h"
#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>

// --- ScriptRunner Base Class ---

ScriptRunner::ScriptRunner(StateMachine& machine, ScriptDefinition& script)
    : m_state_machine(machine), m_script(script) {}

void ScriptRunner::run(const std::map<std::string, std::string>& extra_vars, bool use_temp_cfg) {
    auto& engine = m_state_machine.get_engine();
    auto& ctx = m_state_machine.get_context();
    
    // Create the error result upfront
    m_result.return_code = -1;

    m_cancel_file_path = fs::temp_directory_path() / fs::unique_path("esmm-cancel-%%%%");
    
    fs::path temp_dir;
    std::map<std::string, std::string> final_vars = extra_vars;
    final_vars["$CANCEL_FILE"] = m_cancel_file_path.string();

    if (use_temp_cfg) {
        temp_dir = fs::temp_directory_path() / fs::unique_path();
        fs::create_directories(temp_dir);
        fs::path temp_cfg_path = temp_dir / "openmw.cfg";
        if (!engine.write_temporary_cfg(temp_cfg_path)) {
            m_result.output = "Failed to write temporary config";
            on_finish(-1);
            return;
        }
        final_vars["$OPENMW_CFG"] = temp_cfg_path.string();
    }
    
    std::string command_str = engine.get_script_manager_mut()->build_command_string(m_script, ctx, final_vars);
    command_str += " 2>&1";

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        m_result.output = "Failed to create pipe";
        on_finish(-1);
        return; // Return without a value
    }

    m_pid = fork();
    if (m_pid == -1) {
        m_result.output = "Failed to fork process";
        on_finish(-1);
        return; // Return without a value
    }

    if (m_pid == 0) { // Child process
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        close(pipe_fd[1]);
        
        // Use /bin/sh -c to handle complex commands and arguments correctly
        execl("/bin/sh", "sh", "-c", command_str.c_str(), (char*) NULL);
        _exit(127); // If execl fails
    }
    
    // Parent process
    close(pipe_fd[1]);
    
    engine.add_running_script(this); // Register with engine for cleanup

    FILE* pipe_stream = fdopen(pipe_fd[0], "r");

    // --- THIS IS THE NEW, ROBUST READING LOOP ---
    std::string line_buffer;
    std::string full_output;
    int ch;
    while ((ch = fgetc(pipe_stream)) != EOF) {
        full_output += static_cast<char>(ch);
        if (ch == '\n') { // Only process on newline
            parse_line(line_buffer);
            line_buffer.clear();
        } else if (ch != '\r') { // Ignore carriage returns
            line_buffer += static_cast<char>(ch);
        }
    }

    if (!line_buffer.empty()) { // Process final line if no trailing newline
        parse_line(line_buffer);
    }
    // --- END OF NEW LOOP ---

    int status;
    waitpid(m_pid, &status, 0);
    int return_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    
    on_finish(return_code);

    m_result.return_code = return_code;
    m_result.output = full_output;
    if (use_temp_cfg) {
        m_result.modified_cfg_path = std::make_unique<fs::path>(temp_dir / "openmw.cfg");
    }
    
    // Temp dir cleanup now belongs to the caller of the synchronous run
    // for (auto runner) we'll handle it there. For UIScenes, the scene exit handles it.
    // For simplicity, we'll keep cleanup here for now.

    // FUCK YOU.
    // if (!temp_dir.empty()) {
    //     fs::remove_all(temp_dir);
    // }
}

void ScriptRunner::request_cancellation() {
    if (!m_cancel_file_path.empty()) {
        LOG_INFO("Cancellation requested. Creating cancel file: ", m_cancel_file_path.string());
        std::ofstream cancel_file(m_cancel_file_path.string());
        cancel_file.close();
    }
}

void ScriptRunner::parse_line(const std::string& line) {
    if (line.rfind("ESMM::", 0) != 0) {
        on_line_received(line + "\n");
        return;
    }

    std::string payload = line.substr(6);

    size_t d1 = payload.find("::");
    if (d1 == std::string::npos) return;
    
    std::string key = payload.substr(0, d1);
    std::string val = payload.substr(d1 + 2);

    if (key == "PROGRESS") {
        size_t d2 = val.find("::");
        if (d2 == std::string::npos) return;
        std::string progress_part = val.substr(0, d2);
        std::string rest = val.substr(d2 + 2);

        size_t d3 = rest.find("::");
        if (d3 == std::string::npos) return;
        std::string task_part = rest.substr(0, d3);
        std::string item_part = rest.substr(d3 + 2);

        size_t slash_pos = progress_part.find('/');
        if (slash_pos == std::string::npos) return;
        std::string step_str = progress_part.substr(0, slash_pos);
        std::string total_str = progress_part.substr(slash_pos + 1);

        try {
            m_progress.step = std::stoi(step_str);
            m_progress.total = std::stoi(total_str);
            m_progress.name = task_part;
            m_progress.message = item_part;
            on_progress_update();
        } catch(...) { /* Ignore malformed progress */ }

    } else if (key == "ALERT") {
        size_t d2 = val.find("::");
        if (d2 == std::string::npos) return;
        std::string title = val.substr(0, d2);
        std::string message = val.substr(d2 + 2);
        on_alert({title, message});
    }
}


void ScriptRunner::on_line_received(const std::string& line) {
    // We trim here to avoid printing empty lines with just [SCRIPT]
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    if (!trimmed.empty()) {
        LOG_RAW("[SCRIPT] ", line);
    }
}

void ScriptRunner::on_progress_update() {}
void ScriptRunner::on_alert(const AlertInfo& alert) {}
void ScriptRunner::on_finish(int return_code) {
    m_is_finished = true;
}


// --- HeadlessScriptRunner ---
void HeadlessScriptRunner::on_alert(const AlertInfo& alert) {
    m_alert_triggered = true;
    LOG_WARN("[SCRIPT ALERT] [", alert.title, "]: ", alert.message);

    auto alert_scene = std::make_unique<AlertScene>(m_state_machine, alert.title, alert.message);
    size_t stack_size = m_state_machine.get_stack_size();
    m_state_machine.push_scene(std::move(alert_scene));

    while (m_state_machine.get_stack_size() > stack_size) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// --- UIScriptRunner ---
UIScriptRunner::UIScriptRunner(StateMachine& machine, ScriptDefinition& script)
    : ScriptRunner(machine, script) {}

// NEW: Link the scene to the runner
void UIScriptRunner::set_scene_ptr(ScriptRunnerScene* scene_ptr) {
    m_scene_ptr = scene_ptr;
}


void UIScriptRunner::on_line_received(const std::string& line) {
    ScriptRunner::on_line_received(line);

    if (m_scene_ptr) {
        std::lock_guard<std::mutex> lock(m_scene_ptr->m_log_mutex);
        m_scene_ptr->m_log_lines.push_back(line);
    }
}

void UIScriptRunner::on_progress_update() {
    if (m_scene_ptr) {
        std::lock_guard<std::mutex> lock(m_scene_ptr->m_progress_mutex);
        m_scene_ptr->m_progress = m_progress;
    }
}

void UIScriptRunner::on_alert(const AlertInfo& alert) {
    if (m_scene_ptr) {
        m_scene_ptr->m_alert = alert; // Just copy the whole struct
        m_scene_ptr->m_show_alert = true;
    }
}

void UIScriptRunner::on_finish(int return_code) {
    ScriptRunner::on_finish(return_code); // Call base implementation
    if (m_scene_ptr) m_scene_ptr->m_is_finished = true;
}

