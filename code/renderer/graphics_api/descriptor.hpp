#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/limits.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/vulkan.hpp"
#include <string>
#include <vector>

namespace PG
{
namespace Gfx
{

    class DescriptorSet
    {
        friend class DescriptorPool;
    public:
        DescriptorSet() = default;

        VkDescriptorSet GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;
    };

    static_assert( sizeof( DescriptorSet ) == sizeof( VkDescriptorSet ), "Some functions, like BindDescriptorSet rely on this" );

    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount = 1, uint32_t arrayElement = 0 );
    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1, uint32_t arrayElement = 0 );
    VkDescriptorImageInfo DescriptorImageInfo( const Gfx::Texture& tex, VkImageLayout imageLayout );
    VkDescriptorBufferInfo DescriptorBufferInfo( const Gfx::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE );

    class DescriptorSetLayout
    {
        friend class Device;
    public:
        DescriptorSetLayout() = default;

        void Free();
        VkDescriptorSetLayout GetHandle() const;
        operator bool() const;

        uint32_t sampledImageMask       = 0; // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        uint32_t separateImageMask      = 0; // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
	    uint32_t storageImageMask       = 0; // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
	    uint32_t uniformBufferMask      = 0; // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	    uint32_t storageBufferMask      = 0; // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	    uint32_t uniformTexelBufferMask = 0; // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
	    uint32_t storageTexelBufferMask = 0; // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
	    uint32_t samplerMask            = 0; // VK_DESCRIPTOR_TYPE_SAMPLER
        uint32_t inputAttachmentMask    = 0; // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
        uint32_t arraySizes[PG_MAX_NUM_BINDINGS_PER_SET] = {};

    private:
        VkDescriptorSetLayout m_handle = VK_NULL_HANDLE;
        VkDevice m_device              = VK_NULL_HANDLE;
    };

    void FreeDescriptorSetLayouts( size_t numLayouts, DescriptorSetLayout* layouts );

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
