#include "VulkanRenderer.h"

#include <fstream>
#include <vector>

namespace Graphics::Vulkan
{
    bool VulkanRenderer::Initialize(HWND window, uint32_t renderWidth, uint32_t renderHeight)
    {
        width = renderWidth;
        height = renderHeight;

        if (!context.Initialize(window))
            return false;

        if (!CreatePipeline())
        {
            Shutdown();
            return false;
        }

        return true;
    }

    bool VulkanRenderer::LoadSpirv(const char* path, std::vector<char>& data)
    {
        const char* candidates[] = {
            path,
            "shaders/triangle.vert.spv",
            "../shaders/triangle.vert.spv"
        };

        for (const char* candidate : candidates)
        {
            std::ifstream file(candidate, std::ios::binary | std::ios::ate);
            if (!file)
                continue;

            const std::streamsize size = file.tellg();
            if (size <= 0)
                continue;

            file.seekg(0, std::ios::beg);
            data.resize(static_cast<size_t>(size));

            if (file.read(data.data(), size))
                return true;
        }

        return false;
    }

    VkShaderModule VulkanRenderer::CreateShaderModule(
        const std::vector<char>& code) const
    {
        if (code.size() % 4 != 0)
            return VK_NULL_HANDLE;

        VkShaderModuleCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        info.codeSize = code.size();
        info.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule module = VK_NULL_HANDLE;
        if (vkCreateShaderModule(
                context.GetDevice(),
                &info,
                nullptr,
                &module) != VK_SUCCESS)
            return VK_NULL_HANDLE;

        return module;
    }

    bool VulkanRenderer::CreatePipeline()
    {
        std::vector<char> vertexCode;
        std::vector<char> fragmentCode;

        if (!LoadSpirv("shaders/triangle.vert.spv", vertexCode))
            return false;
        if (!LoadSpirv("shaders/triangle.frag.spv", fragmentCode))
            return false;

        VkShaderModule vertexModule = CreateShaderModule(vertexCode);
        VkShaderModule fragmentModule = CreateShaderModule(fragmentCode);

        if (!vertexModule || !fragmentModule)
            return false;

        VkPipelineShaderStageCreateInfo vertexStage{};
        vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexStage.module = vertexModule;
        vertexStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentStage{};
        fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentStage.module = fragmentModule;
        fragmentStage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = {
            vertexStage, fragmentStage
        };

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType =
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width = static_cast<float>(context.GetExtent().width);
        viewport.height = static_cast<float>(context.GetExtent().height);
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.extent = context.GetExtent();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType =
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType =
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &colorBlendAttachment;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(
                context.GetDevice(),
                &layoutInfo,
                nullptr,
                &pipelineLayout) != VK_SUCCESS)
        {
            vkDestroyShaderModule(context.GetDevice(), vertexModule, nullptr);
            vkDestroyShaderModule(context.GetDevice(), fragmentModule, nullptr);
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType =
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = context.GetRenderPass();
        pipelineInfo.subpass = 0;

        const VkResult result = vkCreateGraphicsPipelines(
            context.GetDevice(),
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &graphicsPipeline);

        vkDestroyShaderModule(context.GetDevice(), vertexModule, nullptr);
        vkDestroyShaderModule(context.GetDevice(), fragmentModule, nullptr);

        return result == VK_SUCCESS;
    }

    void VulkanRenderer::Render(const Adv::AdvControl& control)
    {
        (void)control;

        uint32_t imageIndex = 0;
        if (!context.BeginFrame(imageIndex))
            return;

        VkCommandBuffer commandBuffer = context.GetCommandBuffer();

        VkClearValue clear{};
        clear.color = {{0.02f, 0.025f, 0.04f, 1.0f}};

        VkRenderPassBeginInfo renderPass{};
        renderPass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPass.renderPass = context.GetRenderPass();
        renderPass.framebuffer = context.GetFramebuffer(imageIndex);
        renderPass.renderArea.extent = context.GetExtent();
        renderPass.clearValueCount = 1;
        renderPass.pClearValues = &clear;

        vkCmdBeginRenderPass(
            commandBuffer,
            &renderPass,
            VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        context.EndFrame(imageIndex);
    }

    void VulkanRenderer::Shutdown()
    {
        if (context.GetDevice() != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(context.GetDevice());

            if (graphicsPipeline)
                vkDestroyPipeline(
                    context.GetDevice(), graphicsPipeline, nullptr);

            if (pipelineLayout)
                vkDestroyPipelineLayout(
                    context.GetDevice(), pipelineLayout, nullptr);
        }

        graphicsPipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;

        context.Shutdown();
    }
}
