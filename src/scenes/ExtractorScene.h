#pragma once
#include "Scene.h"
#include "mod/ArchiveManager.h" // For ArchiveInfo
#include <vector>
#include <string>
#include <thread>
#include <mutex>

class ExtractorScene : public Scene {
public:
    // NEW: Constructor to accept a list of archives
    ExtractorScene(std::vector<ArchiveInfo> archives_to_extract);
    ~ExtractorScene();

    void on_enter(AppContext& ctx) override;
    void handle_event(SDL_Event& e, AppContext& ctx) override;
    void render(AppContext& ctx) override;
    void on_exit(AppContext& ctx) override;

private:
    std::vector<ArchiveInfo> archives_to_extract; // Store the list
    std::vector<std::string> log_lines;
    std::mutex log_mutex;
    std::thread worker_thread;
    bool is_finished = false;
    std::string status_message = "Initializing...";

    void extraction_worker(AppContext& ctx);
};
