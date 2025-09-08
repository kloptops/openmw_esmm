#include "ScriptRunnerScene.h"
#include "ScriptRunner.h"
#include "../utils/Logger.h"
#include "../core/StateMachine.h"
#include "../AppContext.h"
#include "imgui.h"
#include <thread>
#include <iostream>
#include <boost/filesystem/path.hpp>
#include <fstream>

namespace fs = boost::filesystem;

ScriptRunnerScene::ScriptRunnerScene(StateMachine& machine, std::shared_ptr<UIScriptRunner> owner, ScriptDefinition& script, bool use_temp_cfg)
    : Scene(machine), m_owner(std::move(owner)), m_script(script), m_use_temp_cfg(use_temp_cfg) {}

void ScriptRunnerScene::on_enter() {
    m_owner->set_scene_ptr(this);
    std::thread([owner = m_owner, use_temp_cfg = m_use_temp_cfg]() {
        owner->run({}, use_temp_cfg); // Use the member flag
    }).detach();
}



void ScriptRunnerScene::render() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin(m_script.title.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

    float bottom_bar_height = 50.0f;

    // --- Main Content Area ---
    ImGui::BeginChild("MainContent", ImVec2(0, -bottom_bar_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (m_script.has_progress) {
        std::lock_guard<std::mutex> lock(m_progress_mutex);
        ImGui::Text("%s", m_progress.name.c_str());
        float progress_fraction = (m_progress.total > 0) ? (float)m_progress.step / m_progress.total : 0.0f;
        ImGui::ProgressBar(progress_fraction, ImVec2(-1, 0));
        ImGui::Text("%s (%d / %d)", m_progress.message.c_str(), m_progress.step, m_progress.total);
        ImGui::Separator();
    }

    if (m_script.has_output) {
        ImGui::BeginChild("LogPanel", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        {
            std::lock_guard<std::mutex> lock(m_log_mutex);
            for (const auto& line : m_log_lines) {
                // Use TextUnformatted for raw output from scripts
                ImGui::TextUnformatted(line.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
    }
    
    ImGui::EndChild(); // End MainContent

    // --- Bottom Bar ---
    ImGui::Separator();
    
    if (m_is_finished) {
        if (ImGui::Button("Back", ImVec2(-1, 40))) {
            m_state_machine.pop_state();
        }
    } else if (m_script.can_cancel) {
        // Re-enable the cancel button and hook it up
        if (ImGui::Button("Cancel", ImVec2(-1, 40))) {
            m_owner->request_cancellation();
        }
    } else {
        ImGui::Dummy(ImVec2(-1, 40));
    }
    
    // --- Alert Modal ---
    if (m_show_alert) {
        ImGui::OpenPopup(m_alert.title.c_str());
        m_show_alert = false; // Reset trigger
    }

    // Use a unique ID for the popup in case of multiple alerts
    if (ImGui::BeginPopupModal((m_alert.title + "##AlertPopup").c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_alert.message.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) { 
            ImGui::CloseCurrentPopup(); 
        }
        ImGui::SetItemDefaultFocus();
        ImGui::EndPopup();
    }

    ImGui::End();
}
