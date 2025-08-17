#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

struct AppContext {
    // SDL & State
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    SDL_GameController* controller = nullptr;
    bool running = true;
    int exit_code = 0;

    // Global Configuration Paths
    fs::path exec_7zz;
    fs::path path_mod_archives;
    fs::path path_mod_data;
    fs::path path_openmw_cfg;
    fs::path path_openmw_esmm_ini;

    ~AppContext();
};
