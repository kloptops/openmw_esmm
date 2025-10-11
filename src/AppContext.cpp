#include "AppContext.h"
#include <iostream>
#include "core/ModEngine.h"
#include "scenes/ScriptRunner.h"
#include "utils/Logger.h"

AppContext::~AppContext()
{
    // --- Orphaned process cleanup ---
    if (engine) { // engine is a new member of AppContext
        for (ScriptRunner* runner : engine->get_running_scripts()) {
            pid_t pid = runner->get_pid();
            if (pid > 0) {
                LOG_WARN("Terminating orphaned script process with PID: ", pid);
                kill(pid, SIGTERM);
            }
        }
    }

    if (controller)
    {
        SDL_GameControllerClose(controller);
    }

    if (font)
    {
        TTF_CloseFont(font);
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }

    if (window)
    {
        SDL_DestroyWindow(window);
    }

    TTF_Quit();
    SDL_Quit();
    std::cout << "AppContext cleaned up." << std::endl;
}
