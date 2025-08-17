#include "MainMenuScene.h"
#include "SettingsScene.h"
#include "../AppContext.h"

SettingsScene::SettingsScene() : MenuScene("OpenMW ESMM")
{
    options = {
        "Reset Config",
        "Back",
    };
}

void SettingsScene::on_select(int index, AppContext& ctx)
{
    switch (index)
    {
    case 0: // Reset Config
        // Do nothing today.
        break;
    case 1: // Extract Mods
        next_scene = std::make_unique<MainMenuScene>();
        break;
    }
}
