#include "ExtractorScene.h"
#include "../core/StateMachine.h"
#include "../AppContext.h"
#include "imgui.h"
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <cstdio>
#include <fstream>

namespace fs = boost::filesystem;

ExtractorScene::ExtractorScene(StateMachine& machine, std::vector<ArchiveInfo> archives) 
    : Scene(machine), m_archives_to_extract(std::move(archives)) {}

ExtractorScene::~ExtractorScene() {
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
}

void ExtractorScene::on_enter() {
    // We pass a const reference to the context from the state machine
    m_worker_thread = std::thread(&ExtractorScene::extraction_worker, this, std::cref(m_state_machine.get_context()));
}

void ExtractorScene::on_exit() {
    if (m_worker_thread.joinable()) {
        m_worker_thread.join();
    }
}

void ExtractorScene::handle_event(SDL_Event& e) {
    if (m_is_finished) {
        if ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) ||
            (e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_B)) {
            // Pop this scene and return to the previous one (ModManagerScene)
            m_state_machine.pop_state();
        }
    }
}

void ExtractorScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("Extractor", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    ImGui::Text("%s", m_status_message.c_str());
    ImGui::Separator();

    ImGui::BeginChild("LogPanel", ImVec2(0, -50), true, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard<std::mutex> lock(m_log_mutex);
        for (const auto& line : m_log_lines) {
            ImGui::TextWrapped("%s", line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    if (m_is_finished) {
        if (ImGui::Button("Back to Mod Manager", ImVec2(ImGui::GetContentRegionAvail().x, 40))) {
            m_state_machine.pop_state();
        }
    }

    ImGui::End();
}

void ExtractorScene::extraction_worker(const AppContext& ctx) {
    auto add_log = [&](const std::string& msg) {
        std::lock_guard<std::mutex> lock(m_log_mutex);
        std::cout << msg << std::endl;
        m_log_lines.push_back(msg);
        m_status_message = "Extracting...";
    };

    try {
        const std::string SEVEN_ZIP_EXEC = ctx.exec_7zz.string();
        const fs::path MOD_DATA_PATH = ctx.path_mod_data;

        add_log("Starting mod extraction process...");
        if (m_archives_to_extract.empty()) {
            add_log("No archives selected for extraction.");
            m_status_message = "Finished: Nothing to extract.";
            m_is_finished = true;
            return;
        }

        fs::create_directories(MOD_DATA_PATH);

        add_log("Found " + std::to_string(m_archives_to_extract.size()) + " archives to process.");

        for (const auto& archive_info : m_archives_to_extract) {
            fs::path output_dir = archive_info.target_data_path;

            add_log("------------------------------------------");
            add_log("Processing: " + archive_info.archive_path.filename().string());
            add_log("-> Target: " + output_dir.string());

            if (fs::exists(output_dir)) {
                add_log("Removing existing directory...");
                boost::system::error_code ec;
                fs::remove_all(output_dir, ec);
                if (ec) {
                    add_log("ERROR removing dir: " + ec.message());
                }
            }

            std::string command = "\"" + SEVEN_ZIP_EXEC + "\" x \"" + archive_info.archive_path.string() + "\" -o\"" + output_dir.string() + "\" -y 2>&1";
            add_log("Executing: " + command);
            
            FILE* pipe = popen(command.c_str(), "r");
            if (!pipe) {
                add_log("ERROR: Failed to run command!");
                continue;
            }

            std::string current_line;
            int ch;
            while ((ch = fgetc(pipe)) != EOF) {
                if (ch == '\n' || ch == '\r') {
                    if (!current_line.empty()) {
                        add_log("  " + current_line);
                        current_line.clear();
                    }
                } else {
                    current_line += static_cast<char>(ch);
                }
            }
            if (!current_line.empty()) {
                add_log("  " + current_line);
            }
            pclose(pipe);

            // --- NEW: Write the marker file on success ---
            fs::create_directories(output_dir); // Ensure it exists
            fs::path marker_file_path = output_dir / "esmm_archive.txt";
            std::ofstream marker_file(marker_file_path.string());
            if (marker_file.is_open()) {
                marker_file << archive_info.archive_path.filename().string();
                marker_file.close();
                add_log("-> Wrote version marker file.");
            } else {
                add_log("!! ERROR: Could not write version marker file to " + marker_file_path.string());
            }
        }

        add_log("------------------------------------------");
        add_log("Extraction complete!");

    } catch (const fs::filesystem_error& e) {
        add_log("Filesystem Error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        add_log("General Error: " + std::string(e.what()));
    }
    
    m_status_message = "Finished. Press (B) to return.";
    m_is_finished = true;
}
