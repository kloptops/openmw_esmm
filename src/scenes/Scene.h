#pragma once
#include <memory>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

// Forward declarations
struct AppContext;
class Scene;

// Global scene manager
extern std::unique_ptr<Scene> next_scene;

// Base class for all scenes
class Scene
{
public:
    virtual ~Scene() = default;
    virtual void on_enter(AppContext& ctx) {}
    virtual void handle_event(SDL_Event& e, AppContext& ctx) = 0;
    virtual void update(AppContext& ctx) {}
    virtual void render(AppContext& ctx) = 0;
    virtual void on_exit(AppContext& ctx) {}
};

// Reusable base class for menu-like scenes
class MenuScene : public Scene
{
protected:
    std::string title;
    std::vector<std::string> options;
    int selected_index = 0;

public:
    MenuScene(const std::string& t);
    virtual ~MenuScene() = default;

    void handle_event(SDL_Event& e, AppContext& ctx) override;
    void render(AppContext& ctx) override;

    virtual void on_select(int index, AppContext& ctx) = 0;
    virtual void on_back(AppContext& ctx) {}
};
