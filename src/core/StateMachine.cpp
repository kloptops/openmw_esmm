#include "StateMachine.h"
#include "../scenes/MainMenuScene.h"
#include "../scenes/ModManagerScene.h"
#include "../scenes/SettingsScene.h"
#include "../scenes/ExtractorScene.h"

StateMachine::StateMachine(AppContext& ctx) : m_context(ctx), m_engine(ctx) {
    m_engine.initialize();
}

void StateMachine::handle_event(SDL_Event& e) {
    if (!m_states.empty()) m_states.back()->handle_event(e);
}

void StateMachine::update() {
    if (!m_states.empty()) m_states.back()->update();
    process_state_changes();
}

void StateMachine::render() {
    if (!m_states.empty()) m_states.back()->render();
}

void StateMachine::push_state(StateID id) { m_pending_changes.push_back({PendingChange::Push, id}); }
void StateMachine::pop_state() { m_pending_changes.push_back({PendingChange::Pop, StateID::MainMenu}); }
void StateMachine::change_state(StateID id) { m_pending_changes.push_back({PendingChange::Change, id}); }

void StateMachine::push_extractor_scene(const std::vector<ArchiveInfo>& archives) {
    // We create the scene here and flag it to be pushed.
    m_extractor_scene_to_push = std::make_unique<ExtractorScene>(*this, archives);
    m_pending_changes.push_back({PendingChange::Push, StateID::Extractor});
}

std::unique_ptr<Scene> StateMachine::create_scene(StateID id) {
    switch (id) {
        case StateID::MainMenu:   return std::make_unique<MainMenuScene>(*this);
        case StateID::ModManager: return std::make_unique<ModManagerScene>(*this);
        case StateID::Settings:   return std::make_unique<SettingsScene>(*this);
        default: return nullptr;
    }
}

void StateMachine::process_state_changes() {
    for (const auto& change : m_pending_changes) {
        switch (change.action) {
            case PendingChange::Push:
                if (change.id == StateID::Extractor && m_extractor_scene_to_push) {
                    m_states.push_back(std::move(m_extractor_scene_to_push));
                } else {
                    m_states.push_back(create_scene(change.id));
                }
                if (m_states.back()) m_states.back()->on_enter();
                break;
            case PendingChange::Pop:
                if (!m_states.empty()) {
                    m_states.back()->on_exit();
                    m_states.pop_back();
                }
                break;
            case PendingChange::Change:
                if (!m_states.empty()) {
                    m_states.back()->on_exit();
                    m_states.pop_back();
                }
                m_states.push_back(create_scene(change.id));
                if (m_states.back()) m_states.back()->on_enter();
                break;
        }
    }
    m_pending_changes.clear();
    if (m_states.empty()) m_context.running = false;
}
