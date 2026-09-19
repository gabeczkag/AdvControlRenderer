#include "Renderer.h"

namespace Graphics
{
    bool Renderer::Initialize(HWND window, uint32_t width, uint32_t height)
    {
        return backend.Initialize(window, width, height);
    }

    void Renderer::Render(const Adv::AdvControl& control)
    {
        backend.Render(control);
    }

    void Renderer::Shutdown()
    {
        backend.Shutdown();
    }
}
