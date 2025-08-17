#pragma once
#include "Scene.h"

class MainMenuScene : public MenuScene
{
public:
    MainMenuScene();
    void on_select(int index, AppContext& ctx) override;
};
