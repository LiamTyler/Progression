// Just do "#include "pg_additions.h" at the bottom of imgui_impl_vulkan.cpp
VkDescriptorSetLayout ImGui_ImplVulkan_GetDescSetLayout()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->DescriptorSetLayout;
}
VkPipelineLayout ImGui_ImplVulkan_GetPipelineLayout()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->PipelineLayout;
}
VkPipeline ImGui_ImplVulkan_GetPipeline()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->Pipeline;
}
VkShaderModule ImGui_ImplVulkan_GetVertShaderModule()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->ShaderModuleVert;
}
VkShaderModule ImGui_ImplVulkan_GetFragShaderModule()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->ShaderModuleFrag;
}
VkDescriptorPool ImGui_ImplVulkan_GetDescriptorPool()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->DescriptorPool;
}
VkDeviceMemory ImGui_ImplVulkan_GetFont_Memory()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->FontTexture.Memory;
}
VkImage ImGui_ImplVulkan_GetFont_Image()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->FontTexture.Image;
}
VkImageView ImGui_ImplVulkan_GetFont_ImageView()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->FontTexture.ImageView;
}
VkDescriptorSet ImGui_ImplVulkan_GetFont_DescriptorSet()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->FontTexture.DescriptorSet;
}
VkSampler ImGui_ImplVulkan_GetTexSampler()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->TexSampler;
}
VkCommandPool ImGui_ImplVulkan_GetTexCommandPool()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->TexCommandPool;
}
VkCommandBuffer ImGui_ImplVulkan_GetTexCommandBuffer()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    return bd->TexCommandBuffer;
}
