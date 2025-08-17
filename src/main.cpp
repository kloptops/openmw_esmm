#include <iostream>
#include "AppContext.h"
#include "utils/Utils.h"
#include "scenes/Scene.h"
#include "scenes/MainMenuScene.h"

// ImGui Includes
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

// Boost Includes for command-line parsing
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char* argv[]) {
    // --- Get Base Path from Executable Location ---
    // This is much more reliable than using "."
    fs::path base_path;
    if (argc > 0 && argv[0]) {
        base_path = fs::system_complete(argv[0]).parent_path();
    } else {
        base_path = fs::current_path();
    }
    std::cout << "Base path detected as: " << base_path.string() << std::endl;
    
    // --- Command-Line Argument Parsing ---
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help",                                   "show help message")
        ("7zz",          po::value<std::string>(), "Path to 7zzs executable")
        ("mod-archives", po::value<std::string>(), "Path to mod archives directory (e.g., mods/)")
        ("mod-data",     po::value<std::string>(), "Path to extracted mod data directory (e.g., mod_data/)")
        ("config-file",  po::value<std::string>(), "Path to openmw.cfg file")
        ("rules-file",   po::value<std::string>(), "Path to openmw_esmm.ini sorting rules")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    // --- Create and Populate AppContext ---
    AppContext ctx;

    // Set paths with priority: command-line > default relative to executable
    ctx.exec_7zz             = vm.count("7zz")          ? fs::path(vm["7zz"].as<std::string>())          : base_path / "7zzs";
    ctx.path_mod_archives    = vm.count("mod-archives") ? fs::path(vm["mod-archives"].as<std::string>()) : base_path / "mods/";
    ctx.path_mod_data        = vm.count("mod-data")     ? fs::path(vm["mod-data"].as<std::string>())     : base_path / "mod_data/";
    ctx.path_openmw_cfg      = vm.count("config-file")  ? fs::path(vm["config-file"].as<std::string>())  : base_path / "openmw/openmw.cfg";
    ctx.path_openmw_esmm_ini = vm.count("rules-file")   ? fs::path(vm["rules-file"].as<std::string>())   : base_path / "openmw_esmm.ini";

    // --- Standard Init ---
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
        return 0;

    if (TTF_Init() == -1)
        return 0;

    ctx.window = SDL_CreateWindow("OpenMW ESMM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    ctx.font = TTF_OpenFont(FONT_PATH, 16);

    if (!ctx.window || !ctx.renderer || !ctx.font)
        return 0;

    // --- ImGui Init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enables keyboard controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enables gamepad controls

    io.IniFilename = NULL; // Disable .ini file saving
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(ctx.window, ctx.renderer);
    ImGui_ImplSDLRenderer2_Init(ctx.renderer); // CHANGED: Call the v2 Init function

    // Controller Init
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i)) {
            ctx.controller = SDL_GameControllerOpen(i);
            if (ctx.controller) break;
        }
    }

    // --- Scene & Main Loop ---
    std::unique_ptr<Scene> current_scene = std::make_unique<MainMenuScene>();
    current_scene->on_enter(ctx);

    while (ctx.running)
    {
        // Event handling
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT)
            {
                ctx.running = false;
                ctx.exit_code = 255;
            }

            // Let the scene handle everything else
            current_scene->handle_event(e, ctx);
        }

        // --- ImGui New Frame ---
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Update
        current_scene->update(ctx);

        // Scene transition
        if (next_scene)
        {
            current_scene->on_exit(ctx);
            current_scene = std::move(next_scene);
            current_scene->on_enter(ctx);
        }

        // --- Rendering ---
        // Let the scene render its ImGui UI
        current_scene->render(ctx);

        // Finalize ImGui render and draw to screen
        SDL_SetRenderDrawColor(ctx.renderer, 20, 20, 40, 255);
        SDL_RenderClear(ctx.renderer);
        ImGui::Render();

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ctx.renderer);
        SDL_RenderPresent(ctx.renderer);
    }

    // --- Shutdown ---
    current_scene->on_exit(ctx);
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    
    int final_exit_code = ctx.exit_code;
    return final_exit_code;
}
