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
    bool mutableSupport  = true;

    void add_binding( uint32_t binding, VkDescriptorType type, uint32_t count );
    void clear();
    VkDescriptorSetLayout build( VkDevice device, VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL );
};

struct DescriptorAllocator
{
    VkDescriptorPool pool;
    VkDevice device;

    void init_pool( VkDevice inDevice, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes );
    void clear_descriptors();
    void destroy_pool();
    VkDescriptorSet allocate( VkDescriptorSetLayout layout );
};

} // namespace PG::Gfx
