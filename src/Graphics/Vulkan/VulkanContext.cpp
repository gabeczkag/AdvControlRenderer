#include "VulkanContext.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <set>
#include <string>

namespace
{
    constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";
    constexpr const char* DeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    bool HasDeviceExtension(VkPhysicalDevice device, const char* requested)
    {
        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

        for (const auto& extension : extensions)
        {
            if (std::strcmp(extension.extensionName, requested) == 0)
                return true;
        }

        return false;
    }
}

namespace Graphics::Vulkan
{
    bool VulkanContext::Initialize(HWND window)
    {
        validationEnabled = false;

#if defined(_DEBUG)
        validationEnabled = CheckValidationLayerSupport();
#endif

        return CreateInstance()
            && CreateSurface(window)
            && PickPhysicalDevice()
            && CreateLogicalDevice()
            && CreateSwapchain()
            && CreateImageViews()
            && CreateRenderPass()
            && CreateCommandPool()
            && CreateCommandBuffer()
            && CreateSyncObjects()
            && CreateFramebuffers();
    }

    bool VulkanContext::CreateInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Adv. Control Renderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.pEngineName = "Adv. Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        };

        if (validationEnabled)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (validationEnabled)
        {
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = &ValidationLayer;
        }

        return vkCreateInstance(&createInfo, nullptr, &instance) == VK_SUCCESS;
    }

    bool VulkanContext::CreateSurface(HWND window)
    {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = GetModuleHandleW(nullptr);
        surfaceInfo.hwnd = window;

        return vkCreateWin32SurfaceKHR(
            instance,
            &surfaceInfo,
            nullptr,
            &surface) == VK_SUCCESS;
    }

    uint32_t VulkanContext::FindGraphicsQueueFamily() const
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);

        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, properties.data());

        for (uint32_t i = 0; i < count; ++i)
        {
            if (properties[i].queueCount > 0 &&
                (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                return i;
        }

        return std::numeric_limits<uint32_t>::max();
    }

    uint32_t VulkanContext::FindPresentQueueFamily() const
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);

        for (uint32_t i = 0; i < count; ++i)
        {
            VkBool32 supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice, i, surface, &supported);

            if (supported)
                return i;
        }

        return std::numeric_limits<uint32_t>::max();
    }

    bool VulkanContext::PickPhysicalDevice()
    {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);

        if (count == 0)
            return false;

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());

        for (VkPhysicalDevice candidate : devices)
        {
            uint32_t graphics = FindGraphicsQueueFamily();
            (void)graphics;

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);

            uint32_t familyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(
                candidate, &familyCount, nullptr);

            bool hasGraphics = false;
            bool hasPresent = false;

            std::vector<VkQueueFamilyProperties> families(familyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                candidate, &familyCount, families.data());

            for (uint32_t family = 0; family < familyCount; ++family)
            {
                if (families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    hasGraphics = true;

                VkBool32 present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(
                    candidate, family, surface, &present);

                if (present)
                    hasPresent = true;
            }

            bool extensionsSupported = true;
            for (const char* extension : DeviceExtensions)
            {
                if (!HasDeviceExtension(candidate, extension))
                {
                    extensionsSupported = false;
                    break;
                }
            }

            if (hasGraphics && hasPresent && extensionsSupported)
            {
                physicalDevice = candidate;
                return true;
            }
        }

        return false;
    }

    bool VulkanContext::CreateLogicalDevice()
    {
        const uint32_t graphicsFamily = FindGraphicsQueueFamily();
        const uint32_t presentFamily = FindPresentQueueFamily();

        if (graphicsFamily == std::numeric_limits<uint32_t>::max() ||
            presentFamily == std::numeric_limits<uint32_t>::max())
            return false;

        std::set<uint32_t> uniqueFamilies{ graphicsFamily, presentFamily };
        constexpr float priority = 1.0f;

        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        for (uint32_t family : uniqueFamilies)
        {
            VkDeviceQueueCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = family;
            info.queueCount = 1;
            info.pQueuePriorities = &priority;
            queueInfos.push_back(info);
        }

        VkPhysicalDeviceFeatures features{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.pEnabledFeatures = &features;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(
            std::size(DeviceExtensions));
        createInfo.ppEnabledExtensionNames = DeviceExtensions;

        if (validationEnabled)
        {
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = &ValidationLayer;
        }

        if (vkCreateDevice(
                physicalDevice,
                &createInfo,
                nullptr,
                &device) != VK_SUCCESS)
            return false;

        vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
        return true;
    }

    VkSurfaceFormatKHR VulkanContext::ChooseSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& formats) const
    {
        for (const auto& format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }

        return formats.front();
    }

    VkPresentModeKHR VulkanContext::ChoosePresentMode(
        const std::vector<VkPresentModeKHR>& modes) const
    {
        for (VkPresentModeKHR mode : modes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanContext::ChooseExtent(
        const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;

        VkExtent2D extent{1280, 720};
        extent.width = std::clamp(
            extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        extent.height = std::clamp(
            extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);
        return extent;
    }

    bool VulkanContext::CreateSwapchain()
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            physicalDevice, surface, &capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &formatCount, formats.data());

        uint32_t modeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &modeCount, nullptr);
        std::vector<VkPresentModeKHR> modes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, surface, &modeCount, modes.data());

        if (formats.empty() || modes.empty())
            return false;

        VkSurfaceFormatKHR chosenFormat = ChooseSurfaceFormat(formats);
        VkPresentModeKHR chosenMode = ChoosePresentMode(modes);
        VkExtent2D extent = ChooseExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0)
            imageCount = std::min(imageCount, capabilities.maxImageCount);

        const uint32_t graphicsFamily = FindGraphicsQueueFamily();
        const uint32_t presentFamily = FindPresentQueueFamily();
        const uint32_t families[] = { graphicsFamily, presentFamily };

        VkSwapchainCreateInfoKHR info{};
        info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        info.surface = surface;
        info.minImageCount = imageCount;
        info.imageFormat = chosenFormat.format;
        info.imageColorSpace = chosenFormat.colorSpace;
        info.imageExtent = extent;
        info.imageArrayLayers = 1;
        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.preTransform = capabilities.currentTransform;
        info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        info.presentMode = chosenMode;
        info.clipped = VK_TRUE;

        if (graphicsFamily != presentFamily)
        {
            info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            info.queueFamilyIndexCount = 2;
            info.pQueueFamilyIndices = families;
        }
        else
        {
            info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        if (vkCreateSwapchainKHR(
                device,
                &info,
                nullptr,
                &swapchain) != VK_SUCCESS)
            return false;

        vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(
            device, swapchain, &imageCount, swapchainImages.data());

        swapchainFormat = chosenFormat.format;
        swapchainExtent = extent;
        return true;
    }

    bool VulkanContext::CreateImageViews()
    {
        swapchainImageViews.resize(swapchainImages.size());

        for (size_t i = 0; i < swapchainImages.size(); ++i)
        {
            VkImageViewCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            info.image = swapchainImages[i];
            info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            info.format = swapchainFormat;
            info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            info.subresourceRange.baseMipLevel = 0;
            info.subresourceRange.levelCount = 1;
            info.subresourceRange.baseArrayLayer = 0;
            info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(
                    device,
                    &info,
                    nullptr,
                    &swapchainImageViews[i]) != VK_SUCCESS)
                return false;
        }

        return true;
    }

    bool VulkanContext::CreateRenderPass()
    {
        VkAttachmentDescription color{};
        color.format = swapchainFormat;
        color.samples = VK_SAMPLE_COUNT_1_BIT;
        color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorReference{};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &color;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 1;
        info.pDependencies = &dependency;

        return vkCreateRenderPass(
            device, &info, nullptr, &renderPass) == VK_SUCCESS;
    }

    bool VulkanContext::CreateCommandPool()
    {
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        info.queueFamilyIndex = FindGraphicsQueueFamily();

        return vkCreateCommandPool(
            device, &info, nullptr, &commandPool) == VK_SUCCESS;
    }

    bool VulkanContext::CreateCommandBuffer()
    {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = commandPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;

        return vkAllocateCommandBuffers(
            device, &info, &commandBuffer) == VK_SUCCESS;
    }

    bool VulkanContext::CreateSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        return vkCreateSemaphore(
                   device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) == VK_SUCCESS
            && vkCreateSemaphore(
                   device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) == VK_SUCCESS
            && vkCreateFence(
                   device, &fenceInfo, nullptr, &inFlightFence) == VK_SUCCESS;
    }

    bool VulkanContext::CreateFramebuffers()
    {
        framebuffers.resize(swapchainImageViews.size());

        for (size_t i = 0; i < swapchainImageViews.size(); ++i)
        {
            VkImageView attachments[] = { swapchainImageViews[i] };

            VkFramebufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            info.renderPass = renderPass;
            info.attachmentCount = 1;
            info.pAttachments = attachments;
            info.width = swapchainExtent.width;
            info.height = swapchainExtent.height;
            info.layers = 1;

            if (vkCreateFramebuffer(
                    device,
                    &info,
                    nullptr,
                    &framebuffers[i]) != VK_SUCCESS)
                return false;
        }

        return true;
    }

    bool VulkanContext::BeginFrame(uint32_t& imageIndex)
    {
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);

        const VkResult result = vkAcquireNextImageKHR(
            device,
            swapchain,
            UINT64_MAX,
            imageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
            return false;

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            return false;

        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        return vkBeginCommandBuffer(commandBuffer, &begin) == VK_SUCCESS;
    }

    bool VulkanContext::EndFrame(uint32_t imageIndex)
    {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            return false;

        VkPipelineStageFlags waitStage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit{};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &imageAvailableSemaphore;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &commandBuffer;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &renderFinishedSemaphore;

        if (vkQueueSubmit(
                graphicsQueue,
                1,
                &submit,
                inFlightFence) != VK_SUCCESS)
            return false;

        VkPresentInfoKHR present{};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &renderFinishedSemaphore;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &imageIndex;

        const VkResult result =
            vkQueuePresentKHR(presentQueue, &present);

        return result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR;
    }

    bool VulkanContext::CheckValidationLayerSupport() const
    {
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());

        for (const auto& layer : layers)
        {
            if (std::strcmp(layer.layerName, ValidationLayer) == 0)
                return true;
        }

        return false;
    }

    void VulkanContext::Shutdown()
    {
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);

        for (VkFramebuffer framebuffer : framebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffers.clear();

        if (inFlightFence)
            vkDestroyFence(device, inFlightFence, nullptr);
        if (renderFinishedSemaphore)
            vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        if (imageAvailableSemaphore)
            vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

        if (commandPool)
            vkDestroyCommandPool(device, commandPool, nullptr);

        if (renderPass)
            vkDestroyRenderPass(device, renderPass, nullptr);

        for (VkImageView imageView : swapchainImageViews)
            vkDestroyImageView(device, imageView, nullptr);
        swapchainImageViews.clear();

        if (swapchain)
            vkDestroySwapchainKHR(device, swapchain, nullptr);

        if (device)
            vkDestroyDevice(device, nullptr);

        if (surface)
            vkDestroySurfaceKHR(instance, surface, nullptr);

        if (instance)
            vkDestroyInstance(instance, nullptr);

        instance = VK_NULL_HANDLE;
        surface = VK_NULL_HANDLE;
        physicalDevice = VK_NULL_HANDLE;
        device = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
        swapchain = VK_NULL_HANDLE;
        renderPass = VK_NULL_HANDLE;
        commandPool = VK_NULL_HANDLE;
        commandBuffer = VK_NULL_HANDLE;
        inFlightFence = VK_NULL_HANDLE;
        imageAvailableSemaphore = VK_NULL_HANDLE;
        renderFinishedSemaphore = VK_NULL_HANDLE;
    }
}
