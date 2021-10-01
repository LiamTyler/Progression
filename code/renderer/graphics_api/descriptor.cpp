#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/debug_marker.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <algorithm>

namespace PG
{
namespace Gfx
{


    VkDescriptorSet DescriptorSet::GetHandle() const
    {
        return m_handle;
    }


    DescriptorSet::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    VkWriteDescriptorSet WriteDescriptorSet_Buffer( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount, uint32_t arrayElement )
    {
		VkWriteDescriptorSet writeDescriptorSet {};
		writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet          = set.GetHandle();
		writeDescriptorSet.descriptorType  = type;
		writeDescriptorSet.dstBinding      = binding;
		writeDescriptorSet.pBufferInfo     = bufferInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
        writeDescriptorSet.dstArrayElement = arrayElement;

		return writeDescriptorSet;
	}


    VkWriteDescriptorSet WriteDescriptorSet_Image( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount, uint32_t arrayElement )
    {
		VkWriteDescriptorSet writeDescriptorSet {};
		writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet          = set.GetHandle();
		writeDescriptorSet.descriptorType  = type;
		writeDescriptorSet.dstBinding      = binding;
		writeDescriptorSet.pImageInfo      = imageInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
        writeDescriptorSet.dstArrayElement = arrayElement;

		return writeDescriptorSet;
	}


    VkDescriptorImageInfo DescriptorImageInfoNull( VkImageLayout imageLayout )
    {
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = imageLayout;
        imageInfo.sampler     = GetSampler( "nearest_clamped_nearest" )->GetHandle(); // has to be valid handle, but doesnt really matter which
        imageInfo.imageView   = VK_NULL_HANDLE;

        return imageInfo;
    }


    VkDescriptorImageInfo DescriptorImageInfo( const Gfx::Texture& tex, VkImageLayout imageLayout )
    {
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = imageLayout;
        imageInfo.sampler     = tex.GetSampler()->GetHandle();
        imageInfo.imageView   = tex.GetView();

        return imageInfo;
    }


    VkDescriptorBufferInfo DescriptorBufferInfo( const Gfx::Buffer& buffer, VkDeviceSize offset, VkDeviceSize range )
    {
        VkDescriptorBufferInfo info;
        info.buffer = buffer.GetHandle();
        info.offset = offset;
        info.range  = range;

        return info;
    }


    void DescriptorSetLayout::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE );
        vkDestroyDescriptorSetLayout( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    VkDescriptorSetLayout DescriptorSetLayout::GetHandle() const
    {
        return m_handle;
    }


    DescriptorSetLayout::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    void FreeDescriptorSetLayouts( size_t numLayouts, DescriptorSetLayout* layouts )
    {
        for ( size_t i = 0; i < numLayouts; ++i )
        {
            layouts[i].Free();
        }
    }


    void DescriptorPool::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyDescriptorPool( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    std::vector< DescriptorSet > DescriptorPool::NewDescriptorSets( uint32_t numLayouts, const DescriptorSetLayout& layout, const std::string& name ) const
    {
        static_assert( sizeof( DescriptorSet ) == sizeof( VkDescriptorSet ) );
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_handle;
        allocInfo.descriptorSetCount = numLayouts;
        std::vector< VkDescriptorSetLayout > vkLayouts( numLayouts, layout.GetHandle() );
        allocInfo.pSetLayouts        = vkLayouts.data();

        std::vector< DescriptorSet > descriptorSets( numLayouts );
        VK_CHECK_RESULT( vkAllocateDescriptorSets( m_device, &allocInfo, (VkDescriptorSet*) descriptorSets.data() ) );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, for ( uint32_t i = 0; i < numLayouts; ++i ) { PG_DEBUG_MARKER_SET_DESC_SET_NAME( descriptorSets[i], name + " " + std::to_string( i ) ); } );

        return descriptorSets;
    }


    DescriptorSet DescriptorPool::NewDescriptorSet( const DescriptorSetLayout& layout, const std::string& name ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType                = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool       = m_handle;
        allocInfo.descriptorSetCount   = 1;
        VkDescriptorSetLayout vkLayout = layout.GetHandle();
        allocInfo.pSetLayouts          = &vkLayout;

        DescriptorSet descriptorSet;
        VK_CHECK_RESULT( vkAllocateDescriptorSets( m_device, &allocInfo, (VkDescriptorSet*) &descriptorSet.m_handle ) );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_DESC_SET_NAME( descriptorSet, name ) );

        return descriptorSet;
    }
    

    VkDescriptorPool DescriptorPool::GetHandle() const
    {
        return m_handle;
    }


    DescriptorPool::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


} // namespace Gfx
} // namespace PG
