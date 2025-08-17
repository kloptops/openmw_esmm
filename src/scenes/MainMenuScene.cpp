#include "MainMenuScene.h"
#include "ModManagerScene.h"
#include "SettingsScene.h"
#include "../AppContext.h"

MainMenuScene::MainMenuScene() : MenuScene("OpenMW ESMM")
{
    options = {
        "Load Morrowind",
        "Mod Manager",
        "Settings",
        "Quit",
    };
}

void MainMenuScene::on_select(int index, AppContext& ctx)
{
    switch (index)
    {
    case 0: // Load Morrowind
        ctx.running = false;
        ctx.exit_code = 0;
        break;
    case 1: // Mod Manager
        next_scene = std::make_unique<ModManagerScene>();
        break;
    case 2: // Settings
        next_scene = std::make_unique<SettingsScene>();
        break;
    case 3: // Quit
        ctx.running = false;
        ctx.exit_code = 255;
        break;
    }
}
