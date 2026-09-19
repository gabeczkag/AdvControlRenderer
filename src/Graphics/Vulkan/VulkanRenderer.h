#pragma once

#include <vulkan/vulkan.h>
#include <windows.h>
#include <cstdint>

#include "VulkanContext.h"
#include "../AdvControl.h"

namespace Graphics::Vulkan
{
    class VulkanRenderer
    {
    public:
        bool Initialize(HWND window, uint32_t width, uint32_t height);
        void Render(const Adv::AdvControl& control);
        void Shutdown();

    private:
        bool CreatePipeline();
        bool LoadSpirv(const char* path, std::vector<char>& data);
        VkShaderModule CreateShaderModule(const std::vector<char>& code) const;

        VulkanContext context;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;

        uint32_t width = 0;
        uint32_t height = 0;
    };
}
