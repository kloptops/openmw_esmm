#pragma once
#include "Scene.h"
#include "../mod/ModManager.h" // For ModDisplayItem
#include "../mod/ArchiveManager.h" // Add this include
#include "../mod/SortManager.h" // Add this include

class ModManagerScene : public Scene {
public:
    ModManagerScene();
    ~ModManagerScene();
    void on_enter(AppContext& ctx) override;
    void handle_event(SDL_Event& e, AppContext& ctx) override;
    void render(AppContext& ctx) override;
    void on_exit(AppContext& ctx) override;
private:
    struct State;
    State* p_state;
};
