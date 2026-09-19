#include "VulkanRenderer.h"
#include <fstream>
namespace Graphics::Vulkan {
bool VulkanRenderer::Initialize(HWND window,uint32_t,uint32_t){
    if(!context.Initialize(window)) return false;
    if(!CreatePipeline()){ Shutdown(); return false; }
    return true;
}
bool VulkanRenderer::LoadSpirv(const char* path,std::vector<char>& data){
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f) return false;
    auto size=f.tellg(); if(size<=0 || size%4!=0) return false;
    f.seekg(0); data.resize((size_t)size); return !!f.read(data.data(),size);
}
VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code) const{
    VkShaderModuleCreateInfo i{}; i.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    i.codeSize=code.size(); i.pCode=reinterpret_cast<const uint32_t*>(code.data());
    VkShaderModule m=VK_NULL_HANDLE; return vkCreateShaderModule(context.GetDevice(),&i,nullptr,&m)==VK_SUCCESS?m:VK_NULL_HANDLE;
}
bool VulkanRenderer::CreatePipeline(){
    std::vector<char> vs,fs;
    if(!LoadSpirv("shaders/triangle.vert.spv",vs)||!LoadSpirv("shaders/triangle.frag.spv",fs)) return false;
    VkShaderModule v=CreateShaderModule(vs), f=CreateShaderModule(fs); if(!v||!f) return false;
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType=stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stages[0].module=v; stages[0].pName="main";
    stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module=f; stages[1].pName="main";
    VkPipelineVertexInputStateCreateInfo vi{}; vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo ia{}; ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport vp{}; vp.width=(float)context.GetExtent().width; vp.height=(float)context.GetExtent().height; vp.maxDepth=1.0f;
    VkRect2D sc{}; sc.extent=context.GetExtent();
    VkPipelineViewportStateCreateInfo vsInfo{}; vsInfo.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; vsInfo.viewportCount=1; vsInfo.pViewports=&vp; vsInfo.scissorCount=1; vsInfo.pScissors=&sc;
    VkPipelineRasterizationStateCreateInfo rs{}; rs.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_NONE; rs.lineWidth=1.0f;
    VkPipelineMultisampleStateCreateInfo ms{}; ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState cb{}; cb.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cbs{}; cbs.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; cbs.attachmentCount=1; cbs.pAttachments=&cb;
    VkPipelineLayoutCreateInfo li{}; li.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if(vkCreatePipelineLayout(context.GetDevice(),&li,nullptr,&pipelineLayout)!=VK_SUCCESS){vkDestroyShaderModule(context.GetDevice(),v,nullptr);vkDestroyShaderModule(context.GetDevice(),f,nullptr);return false;}
    VkGraphicsPipelineCreateInfo pi{}; pi.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; pi.stageCount=2; pi.pStages=stages; pi.pVertexInputState=&vi; pi.pInputAssemblyState=&ia; pi.pViewportState=&vsInfo; pi.pRasterizationState=&rs; pi.pMultisampleState=&ms; pi.pColorBlendState=&cbs; pi.layout=pipelineLayout; pi.renderPass=context.GetRenderPass();
    auto result=vkCreateGraphicsPipelines(context.GetDevice(),VK_NULL_HANDLE,1,&pi,nullptr,&graphicsPipeline);
    vkDestroyShaderModule(context.GetDevice(),v,nullptr); vkDestroyShaderModule(context.GetDevice(),f,nullptr);
    return result==VK_SUCCESS;
}
void VulkanRenderer::Render(const Adv::AdvControl& control){
    (void)control; uint32_t index=0; if(!context.BeginFrame(index)) return;
    VkCommandBuffer cmd=context.GetCommandBuffer(); VkClearValue clear{}; clear.color={{0.02f,0.025f,0.04f,1.0f}};
    VkRenderPassBeginInfo rp{}; rp.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; rp.renderPass=context.GetRenderPass(); rp.framebuffer=context.GetFramebuffer(index); rp.renderArea.extent=context.GetExtent(); rp.clearValueCount=1; rp.pClearValues=&clear;
    vkCmdBeginRenderPass(cmd,&rp,VK_SUBPASS_CONTENTS_INLINE); vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,graphicsPipeline); vkCmdDraw(cmd,3,1,0,0); vkCmdEndRenderPass(cmd); context.EndFrame(index);
}
void VulkanRenderer::Shutdown(){
    if(context.GetDevice()){vkDeviceWaitIdle(context.GetDevice()); if(graphicsPipeline)vkDestroyPipeline(context.GetDevice(),graphicsPipeline,nullptr); if(pipelineLayout)vkDestroyPipelineLayout(context.GetDevice(),pipelineLayout,nullptr);}
    graphicsPipeline=VK_NULL_HANDLE; pipelineLayout=VK_NULL_HANDLE; context.Shutdown();
}
}