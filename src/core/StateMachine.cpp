#include "StateMachine.h"

StateMachine::StateMachine(AppContext& ctx) : m_context(ctx), m_engine(ctx) {
    m_engine.set_state_machine(*this);
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

void StateMachine::push_scene(std::unique_ptr<Scene> scene) {
    m_pending_changes.push_back({PendingChange::Push, std::move(scene)});
}

void StateMachine::pop_state() {
    m_pending_changes.push_back({PendingChange::Pop, nullptr});
}

void StateMachine::change_scene(std::unique_ptr<Scene> scene) {
    m_pending_changes.push_back({PendingChange::Change, std::move(scene)});
}

void StateMachine::process_state_changes() {
    for (auto& change : m_pending_changes) {
        switch (change.action) {
            case PendingChange::Push:
                if (change.scene) {
                    m_states.push_back(std::move(change.scene));
                    m_states.back()->on_enter();
                }
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
                if (change.scene) {
                    m_states.push_back(std::move(change.scene));
                    m_states.back()->on_enter();
                }
                break;
        }
    }

    m_pending_changes.clear();
    if (m_states.empty()) m_context.running = false;
}
