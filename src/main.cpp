#include <iostream>
#include "AppContext.h"
#include "utils/Logger.h"
#include "utils/Utils.h"
#include "scenes/Scene.h"
#include "scenes/MainMenuScene.h"
#include "mod/ConfigManager.h"
#include "mod/MloxManager.h"

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
        ("config-dir",   po::value<std::string>(), "Directory for esmm configs (ini, mlox)")
        ("quiet",                                  "Quieten down logging")
        ("verbose",                                "Enable verbose debug logging")
        ("debug-sort",                             "Run the mlox sorter on the current config and exit") // <-- NEW
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("quiet")) {
        Logger::get().set_level(LogLevel::WARN);
        // LOG_DEBUG("Quieten logging enabled.");
    }

    if (vm.count("verbose")) {
        Logger::get().set_level(LogLevel::DEBUG);
        LOG_DEBUG("Verbose logging enabled.");
    }


    AppContext tmp_ctx;
    tmp_ctx.path_config_dir = vm.count("config-dir")  ? fs::path(vm["config-dir"].as<std::string>())  : base_path;
    tmp_ctx.path_mod_data   = vm.count("mod-data")    ? fs::path(vm["mod-data"].as<std::string>())    : base_path / "mod_data/";
    tmp_ctx.path_openmw_cfg = vm.count("config-file") ? fs::path(vm["config-file"].as<std::string>()) : base_path / "openmw.cfg";
    
    // --- Debug Sorter Mode ---
    if (vm.count("debug-sort")) {
        ConfigManager cfg;
        MloxManager mlox;

        if (!cfg.load(tmp_ctx.path_openmw_cfg)) {
            std::cerr << "ERROR: Could not load openmw.cfg for debug sort." << std::endl;
            return 1;
        }

        fs::path base_mlox_path = tmp_ctx.path_config_dir / "mlox_base.txt";
        fs::path user_mlox_path = tmp_ctx.path_config_dir / "mlox_user.txt";
        mlox.load_rules({base_mlox_path, user_mlox_path});

        // --- THIS IS THE FIX ---
        // 1. Create a single, combined list of all content files from the config.
        std::vector<std::string> all_content_from_cfg = cfg.base_content;
        all_content_from_cfg.insert(all_content_from_cfg.end(), cfg.mod_content.begin(), cfg.mod_content.end());

        // 2. Create the ContentFile vector for sorting.
        std::vector<ContentFile> content_to_sort;
        for (const auto& name : all_content_from_cfg) {
            content_to_sort.push_back(ContentFile{name, true, "Unknown"});
        }

        // 3. Create the necessary path vectors.
        std::vector<fs::path> base_data_paths;
        for (const auto& s : cfg.base_data) {
            base_data_paths.push_back(fs::path(s));
        }
        std::vector<fs::path> mod_data_paths;
        for (const auto& s : cfg.mod_data) {
            mod_data_paths.push_back(fs::path(s));
        }

        // 4. Call the sorter with the correct arguments.
        mlox.sort_content_files(content_to_sort, base_data_paths, mod_data_paths);
        
        return 0;
    }
    
    // --- REGULAR APPLICATION STARTUP ---
    AppContext& ctx = tmp_ctx;

    // Set paths with priority: command-line > default relative to executable
    ctx.exec_7zz             = vm.count("7zz")          ? fs::path(vm["7zz"].as<std::string>())          : base_path / "7zzs";
    ctx.path_mod_archives    = vm.count("mod-archives") ? fs::path(vm["mod-archives"].as<std::string>()) : base_path / "mods/";

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
