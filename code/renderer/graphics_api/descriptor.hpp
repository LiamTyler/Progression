#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include "renderer/vulkan.hpp"
#include <string>
#include <vector>

namespace PG::Gfx
{

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bool bindlessSupport = true;
    bool mutableSupport  = false;

    void AddBinding( u32 binding, VkDescriptorType type, u32 count );
    void Clear();
    VkDescriptorSetLayout Build( VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL, std::string_view name = "" );
};

struct DescriptorAllocator
{
    VkDescriptorPool pool;

    void Init( u32 maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes, std::string_view name = "" );
    void ClearDescriptors();
    void Free();
    VkDescriptorSet Allocate( VkDescriptorSetLayout layout, std::string_view name = "" );
};

struct DescriptorBuffer
{
    VkDeviceSize layoutSize;
    VkDeviceSize offset;
    Buffer buffer;

    bool Create( VkDescriptorSetLayout layout, std::string_view name = "" );
    void Free();
};

void InitGlobalDescriptorData();
void FreeGlobalDescriptorData();

const VkDescriptorSetLayout& GetGlobalDescriptorSetLayout();
#if USING( PG_DESCRIPTOR_BUFFER )
DescriptorBuffer& GetGlobalDescriptorBuffer();
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
const VkDescriptorSet& GetGlobalDescriptorSet();
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

} // namespace PG::Gfx
