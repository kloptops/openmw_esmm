#pragma once
#include "Scene.h"
#include "../mod/ArchiveManager.h" // For ArchiveInfo
#include <vector>
#include <string>
#include <thread>
#include <mutex>

class ExtractorScene : public Scene {
public:
    // Constructor now takes the StateMachine and the archives
    ExtractorScene(StateMachine& machine, std::vector<ArchiveInfo> archives_to_extract);
    ~ExtractorScene();

    void on_enter() override;
    void on_exit() override;
    void handle_event(SDL_Event& e) override;
    void render() override;

private:
    // Worker function now takes a const reference to AppContext
    void extraction_worker(const AppContext& ctx);

    std::vector<ArchiveInfo> m_archives_to_extract;
    std::thread m_worker_thread;
    std::mutex m_log_mutex;
    std::vector<std::string> m_log_lines;
    std::string m_status_message = "Initializing...";
    bool m_is_finished = false;
};
