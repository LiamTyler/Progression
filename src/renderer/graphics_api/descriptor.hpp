#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/texture.hpp"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace PG
{
namespace Gfx
{

    class DescriptorSet
    {
    public:
        DescriptorSet() = default;

        VkDescriptorSet GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;
    };

    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount = 1, uint32_t arrayElement = 0 );
    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1, uint32_t arrayElement = 0 );
    VkDescriptorImageInfo DescriptorImageInfo( const Gfx::Texture& tex, VkImageLayout imageLayout );
    VkDescriptorBufferInfo DescriptorBufferInfo( const Gfx::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE );

    struct DescriptorSetLayoutData
    {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector< VkDescriptorSetLayoutBinding > bindings;
    };

    std::vector< DescriptorSetLayoutData > CombineDescriptorSetLayouts( std::vector< DescriptorSetLayoutData >& layoutDatas );


    class DescriptorSetLayout
    {
        friend class Device;
    public:
        DescriptorSetLayout() = default;

        void Free();
        VkDescriptorSetLayout GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorSetLayout m_handle = VK_NULL_HANDLE;
        VkDevice m_device              = VK_NULL_HANDLE;
    };

    void FreeDescriptorSetLayouts( size_t numLayouts, DescriptorSetLayout* layouts );
    void FreeDescriptorSetLayouts( std::vector< DescriptorSetLayout >& layouts );


    class DescriptorPool 
    {
        friend class Device;
    public:
        DescriptorPool() = default;

        void Free();
        std::vector< DescriptorSet > NewDescriptorSets( uint32_t numLayouts, const DescriptorSetLayout& layout, const std::string& name = "" ) const;
        DescriptorSet NewDescriptorSet( const DescriptorSetLayout& layout, const std::string& name = "" ) const;
        VkDescriptorPool GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device         = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace PG
