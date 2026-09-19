#include <windows.h>
#include "Core/Application.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
    Application app;
    if (!app.Initialize(instance))
        return -1;

    return app.Run();
}
