#include "AppContext.h"
#include <iostream>

AppContext::~AppContext()
{
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
