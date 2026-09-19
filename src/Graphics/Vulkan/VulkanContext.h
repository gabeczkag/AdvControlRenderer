#pragma once

#include <vulkan/vulkan.h>
#include <windows.h>
#include <cstdint>
#include <vector>

namespace Graphics::Vulkan
{
    class VulkanContext
    {
    public:
        bool Initialize(HWND window);
        void Shutdown();

        bool BeginFrame(uint32_t& imageIndex);
        bool EndFrame(uint32_t imageIndex);

        VkDevice GetDevice() const { return device; }
        VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
        VkQueue GetGraphicsQueue() const { return graphicsQueue; }
        VkQueue GetPresentQueue() const { return presentQueue; }
        VkCommandPool GetCommandPool() const { return commandPool; }
        VkCommandBuffer GetCommandBuffer() const { return commandBuffer; }
        VkRenderPass GetRenderPass() const { return renderPass; }
        VkFramebuffer GetFramebuffer(uint32_t index) const { return framebuffers[index]; }
        VkExtent2D GetExtent() const { return swapchainExtent; }
        VkFormat GetSwapchainFormat() const { return swapchainFormat; }
        uint32_t GetImageCount() const { return static_cast<uint32_t>(swapchainImages.size()); }

        const std::vector<VkImageView>& GetImageViews() const { return swapchainImageViews; }

    private:
        bool CreateInstance();
        bool CreateSurface(HWND window);
        bool PickPhysicalDevice();
        bool CreateLogicalDevice();
        bool CreateSwapchain();
        bool CreateImageViews();
        bool CreateRenderPass();
        bool CreateCommandPool();
        bool CreateCommandBuffer();
        bool CreateSyncObjects();
        bool CreateFramebuffers();

        bool RecreateSwapchain();

        uint32_t FindGraphicsQueueFamily() const;
        uint32_t FindPresentQueueFamily() const;

        VkSurfaceFormatKHR ChooseSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR>& formats) const;

        VkPresentModeKHR ChoosePresentMode(
            const std::vector<VkPresentModeKHR>& modes) const;

        VkExtent2D ChooseExtent(
            const VkSurfaceCapabilitiesKHR& capabilities) const;

        bool CheckValidationLayerSupport() const;

    private:
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;

        VkQueue graphicsQueue = VK_NULL_HANDLE;
        VkQueue presentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        std::vector<VkFramebuffer> framebuffers;

        VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
        VkExtent2D swapchainExtent{};

        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkCommandPool commandPool = VK_NULL_HANDLE;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
        VkFence inFlightFence = VK_NULL_HANDLE;

        bool validationEnabled = false;
    };
}
