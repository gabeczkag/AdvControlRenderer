#pragma once

#include <windows.h>
#include "../Platform/Window.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/AdvControl.h"

class Application
{
public:
    bool Initialize(HINSTANCE instance);
    int Run();

private:
    Window window;
    Graphics::Renderer renderer;
    Adv::AdvControl advControl;
};
