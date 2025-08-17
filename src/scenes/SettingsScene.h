#pragma once
#include "Scene.h"

class SettingsScene : public MenuScene {
public:
    SettingsScene();
    void on_select(int index, AppContext& ctx) override;
};
