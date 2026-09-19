#pragma once

#include <windows.h>
#include <cstdint>
#include "AdvControl.h"
#include "Vulkan/VulkanRenderer.h"

namespace Graphics
{
    // Engine-facing renderer facade. The engine does not know Vulkan details.
    class Renderer
    {
    public:
        bool Initialize(HWND window, uint32_t width, uint32_t height);
        void Render(const Adv::AdvControl& control);
        void Shutdown();

    private:
        Vulkan::VulkanRenderer backend;
    };
}
