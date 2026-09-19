#include "Application.h"

bool Application::Initialize(HINSTANCE instance)
{
    if (!window.Create(instance, 1280, 720, L"Adv. Engine - Vulkan"))
        return false;

    if (!renderer.Initialize(window.GetHandle(), 1280, 720))
        return false;

    return true;
}

int Application::Run()
{
    while (window.ProcessMessages())
    {
        advControl.BeginFrame();

        // CPU-side description. GPU execution is owned by the Vulkan backend.
        advControl.AddTask(Adv::TaskType::Clear);
        advControl.AddTask(Adv::TaskType::Geometry, 3);

        renderer.Render(advControl);
    }

    renderer.Shutdown();
    return 0;
}
