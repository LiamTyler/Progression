#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/limits.hpp"
#include "renderer/graphics_api/texture.hpp"
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

    void AddBinding( uint32_t binding, VkDescriptorType type, uint32_t count );
    void Clear();
    VkDescriptorSetLayout Build( VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL, const std::string& name = "" );
};

struct DescriptorAllocator
{
    VkDescriptorPool pool;

    void Init( uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes, const std::string& name = "" );
    void ClearDescriptors();
    void Free();
    VkDescriptorSet Allocate( VkDescriptorSetLayout layout, const std::string& name = "" );
};

struct DescriptorBuffer
{
    VkDeviceSize layoutSize;
    VkDeviceSize offset;
    Buffer buffer;

    bool Create( VkDescriptorSetLayout layout, const std::string& name = "" );
    void Free();
};

} // namespace PG::Gfx
