#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/debug_marker.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <algorithm>

namespace PG::Gfx
{

void DescriptorLayoutBuilder::AddBinding( uint32_t binding, VkDescriptorType type, uint32_t count )
{
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = binding;
    newbind.descriptorCount = count;
    newbind.descriptorType  = type;

    bindings.push_back( newbind );
}

void DescriptorLayoutBuilder::Clear() { bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::Build( VkDevice device, VkShaderStageFlags shaderStages )
{
    for ( auto& b : bindings )
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.pBindings    = bindings.data();
    info.bindingCount = (uint32_t)bindings.size();
    info.flags        = 0;
    // info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
#if USING( PG_DESCRIPTOR_BUFFER )
    info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    VkDescriptorBindingFlags bindFlag = 0;
    if ( bindlessSupport )
    {
        bindFlag |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        // bindFlag |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        // bindFlag |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        extendedInfo.bindingCount  = 1;
        extendedInfo.pBindingFlags = &bindFlag;
    }

#if USING( PG_MUTABLE_DESCRIPTORS )
    VkDescriptorType mutableTexTypes[] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE };

    VkMutableDescriptorTypeListVALVE descTypeList{};
    descTypeList.descriptorTypeCount = ARRAY_COUNT( mutableTexTypes );
    descTypeList.pDescriptorTypes    = mutableTexTypes;

    VkMutableDescriptorTypeCreateInfoEXT mutableTypeInfo{ VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT };
    mutableTypeInfo.mutableDescriptorTypeListCount = 1;
    mutableTypeInfo.pMutableDescriptorTypeLists    = &descTypeList;

    if ( mutableSupport )
        extendedInfo.pNext = &mutableTypeInfo;
#endif // #if USING( PG_MUTABLE_DESCRIPTORS )

    info.pNext = &extendedInfo;
    VkDescriptorSetLayout setLayout;
    VK_CHECK( vkCreateDescriptorSetLayout( device, &info, nullptr, &setLayout ) );

    return setLayout;
}

void DescriptorAllocator::Init( VkDevice inDevice, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes )
{
    device = inDevice;

    VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    pool_info.flags         = 0; // VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    pool_info.maxSets       = maxSets;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes    = poolSizes.data();

    vkCreateDescriptorPool( device, &pool_info, nullptr, &pool );
}

void DescriptorAllocator::ClearDescriptors() { vkResetDescriptorPool( device, pool, 0 ); }

void DescriptorAllocator::Free() { vkDestroyDescriptorPool( device, pool, nullptr ); }

VkDescriptorSet DescriptorAllocator::Allocate( VkDescriptorSetLayout layout )
{
    VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.pNext                       = nullptr;
    allocInfo.descriptorPool              = pool;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &layout;

    VkDescriptorSet ds;
    VK_CHECK( vkAllocateDescriptorSets( device, &allocInfo, &ds ) );

    return ds;
}

bool DescriptorBuffer::Create( VkDescriptorSetLayout layout, const std::string& name )
{
#if USING( PG_DESCRIPTOR_BUFFER )
    const VkPhysicalDeviceDescriptorBufferPropertiesEXT& dbProps = rg.physicalDevice.GetProperties().dbProps;
    vkGetDescriptorSetLayoutSizeEXT( rg.device, layout, &layoutSize );
    layoutSize = ALIGN_UP_POW_2( layoutSize, dbProps.descriptorBufferOffsetAlignment );
    vkGetDescriptorSetLayoutBindingOffsetEXT( rg.device, layout, 0u, &offset );

    BufferCreateInfo bCI = {};
    bCI.size             = layoutSize;
    bCI.bufferUsage |= BufferUsage::RESOURCE_DESCRIPTOR;
    bCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    buffer = rg.device.NewBuffer( bCI, name );

    return true;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    return false;
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )
}

void DescriptorBuffer::Free() { buffer.Free(); }

} // namespace PG::Gfx
