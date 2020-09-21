#include "renderer/graphics_api/descriptor.hpp"
#include "core/assert.hpp"
#include "renderer/debug_marker.hpp"
#include "utils/logger.hpp"
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


    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount, uint32_t arrayElement )
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


    VkWriteDescriptorSet WriteDescriptorSet( const DescriptorSet& set, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount, uint32_t arrayElement )
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


    VkDescriptorImageInfo DescriptorImageInfo( const Gfx::Texture& tex, VkImageLayout imageLayout )
    {
        VkDescriptorImageInfo imageInfo = {};
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


    std::vector< DescriptorSetLayoutData > CombineDescriptorSetLayouts( std::vector< DescriptorSetLayoutData >& layoutDatas )
    {
        std::sort( layoutDatas.begin(), layoutDatas.end(),
                []( const auto& lhs, const auto& rhs ) { return lhs.setNumber < rhs.setNumber; } );

        std::vector< DescriptorSetLayoutData > combined;
        for ( size_t i = 0; i < layoutDatas.size(); )
        {
            combined.push_back( {} );
            auto& newSet = combined[combined.size() - 1];
            newSet.setNumber = layoutDatas[i].setNumber;
            newSet.bindings  = layoutDatas[i].bindings;
            // For all sets with the same number, add their bindings to the list, either as new entries in the list,
            // or just by |= the stageFlags if its the same binding + type as an existing entry
            ++i;
            while ( i < layoutDatas.size() && layoutDatas[i].setNumber == newSet.setNumber )
            {
                for ( const auto& newBinding : layoutDatas[i].bindings )
                {
                    bool found = false;
                    for ( auto& existingBinding : newSet.bindings )
                    {
                        if ( newBinding.binding == existingBinding.binding )
                        {
                            PG_ASSERT( newBinding.descriptorCount == existingBinding.descriptorCount &&
                                       newBinding.descriptorType == existingBinding.descriptorType,
                                       "Descriptors have same binding number, but different types and/or counts" );
                            existingBinding.stageFlags |= newBinding.stageFlags;
                            found = true;
                        }
                    }
                    if ( !found )
                    {
                        newSet.bindings.push_back( newBinding );
                    }
                }
                ++i;
            }
        }

        for ( auto& set : combined )
        {
            set.createInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            set.createInfo.bindingCount = static_cast< uint32_t >( set.bindings.size() );
            set.createInfo.pBindings    = set.bindings.data();
        }
        
        // for ( size_t i = 0; i < combined.size(); ++i )
        // {
        //     LOG( "combined[" , i, "].setNumber = ", combined[i].setNumber );
        //     for ( size_t b = 0; b < combined[i].bindings.size(); ++b )
        //     {
        //         LOG( "combined[" , i, "].binding[", b, "].binding = ", combined[i].bindings[b].binding );
        //         LOG( "combined[" , i, "].binding[", b, "].descriptorType = ", combined[i].bindings[b].descriptorType );
        //     }
        // }

        return combined;
    }


    void FreeDescriptorSetLayouts( size_t numLayouts, DescriptorSetLayout* layouts )
    {
        for ( size_t i = 0; i < numLayouts; ++i )
        {
            layouts[i].Free();
        }
    }


    void FreeDescriptorSetLayouts( std::vector< DescriptorSetLayout >& layouts )
    {
        FreeDescriptorSetLayouts( layouts.size(), layouts.data() );
    }


    void DescriptorPool::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyDescriptorPool( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    std::vector< DescriptorSet > DescriptorPool::NewDescriptorSets( uint32_t numLayouts, const DescriptorSetLayout& layout, const std::string& name ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_handle;
        allocInfo.descriptorSetCount = numLayouts;
        std::vector< VkDescriptorSetLayout > vkLayouts( numLayouts, layout.GetHandle() );
        allocInfo.pSetLayouts        = vkLayouts.data();

        std::vector< DescriptorSet > descriptorSets( numLayouts );
        VkResult res = vkAllocateDescriptorSets( m_device, &allocInfo, (VkDescriptorSet*) descriptorSets.data() );
        PG_ASSERT( res == VK_SUCCESS );
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
        VkResult res = vkAllocateDescriptorSets( m_device, &allocInfo, (VkDescriptorSet*) &descriptorSet );
        PG_ASSERT( res == VK_SUCCESS );
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
