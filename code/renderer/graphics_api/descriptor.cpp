#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/debug_marker.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <algorithm>

namespace PG::Gfx
{

static VkDescriptorSetLayout s_globalDescriptorSetLayout;
#if USING( PG_DESCRIPTOR_BUFFER )
static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_dbProps;
static DescriptorBuffer s_globalDescriptorBuffer;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
static DescriptorAllocator s_descriptorAllocator;
static VkDescriptorSet s_globalDescriptorSet;
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

void DescriptorLayoutBuilder::AddBinding( u32 binding, VkDescriptorType type, u32 count )
{
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding         = binding;
    newbind.descriptorCount = count;
    newbind.descriptorType  = type;

    bindings.push_back( newbind );
}

void DescriptorLayoutBuilder::Clear() { bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::Build( VkShaderStageFlags shaderStages, std::string_view name )
{
    for ( auto& b : bindings )
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    info.pBindings    = bindings.data();
    info.bindingCount = (u32)bindings.size();
    info.flags        = 0;
    info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
#if USING( PG_DESCRIPTOR_BUFFER )
    info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    std::vector<VkDescriptorBindingFlags> bindFlags( bindings.size(), {} );
    if ( bindlessSupport )
    {
        for ( size_t i = 0; i < bindings.size(); ++i )
        {
            const VkDescriptorSetLayoutBinding& binding = bindings[i];
            VkDescriptorBindingFlags& bindFlag          = bindFlags[i];
            if ( ( binding.descriptorCount == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                     binding.descriptorCount == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ) &&
                 binding.descriptorCount == PG_MAX_BINDLESS_TEXTURES )
            {
                bindFlag |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
                // bindFlag |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
            }
            bindFlag |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        }

        extendedInfo.bindingCount  = (u32)bindings.size();
        extendedInfo.pBindingFlags = bindFlags.data();
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
    VK_CHECK( vkCreateDescriptorSetLayout( rg.device, &info, nullptr, &setLayout ) );
    PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( setLayout, name );

    return setLayout;
}

void DescriptorAllocator::Init( u32 maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes, std::string_view name )
{
    VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    // Even if we can update all the descriptors before the command buffer, the limit counts are much higher with it.
    // Ex: VkPhysicalDeviceVulkan12Properties::maxDescriptorSetUpdateAfterBindStorageImages = 1048576
    // vs VkPhysicalDeviceLimits::maxDescriptorSetStorageImages = 144 on my Intel computer
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets       = maxSets;
    poolInfo.poolSizeCount = (u32)poolSizes.size();
    poolInfo.pPoolSizes    = poolSizes.data();

    vkCreateDescriptorPool( rg.device, &poolInfo, nullptr, &pool );
    PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name );
}

void DescriptorAllocator::ClearDescriptors() { vkResetDescriptorPool( rg.device, pool, 0 ); }

void DescriptorAllocator::Free() { vkDestroyDescriptorPool( rg.device, pool, nullptr ); }

VkDescriptorSet DescriptorAllocator::Allocate( VkDescriptorSetLayout layout, std::string_view name )
{
    VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.pNext                       = nullptr;
    allocInfo.descriptorPool              = pool;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &layout;

    VkDescriptorSet ds;
    VK_CHECK( vkAllocateDescriptorSets( rg.device, &allocInfo, &ds ) );
    PG_DEBUG_MARKER_SET_DESC_SET_NAME( ds, name );

    return ds;
}

bool DescriptorBuffer::Create( VkDescriptorSetLayout layout, std::string_view name )
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

void InitGlobalDescriptorData()
{
    DescriptorLayoutBuilder setLayoutBuilder;
    setLayoutBuilder.AddBinding( PG_BINDLESS_SAMPLED_TEXTURE_DSET_BINDING, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, PG_MAX_BINDLESS_TEXTURES );
    setLayoutBuilder.AddBinding( PG_BINDLESS_STORAGE_IMAGE_DSET_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, PG_MAX_BINDLESS_TEXTURES );
    setLayoutBuilder.AddBinding( PG_SCENE_GLOBALS_DSET_BINDING, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 );
    setLayoutBuilder.AddBinding( PG_BINDLESS_BUFFER_DSET_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 );
    setLayoutBuilder.AddBinding( PG_BINDLESS_MATERIALS_DSET_BINDING, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 );
    setLayoutBuilder.AddBinding( PG_BINDLESS_SAMPLERS_DSET_BINDING, VK_DESCRIPTOR_TYPE_SAMPLER, 12 );
    s_globalDescriptorSetLayout = setLayoutBuilder.Build( VK_SHADER_STAGE_ALL, "global" );

#if USING( PG_DESCRIPTOR_BUFFER )
    s_globalDescriptorBuffer.Create( s_globalDescriptorSetLayout );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,  PG_MAX_BINDLESS_TEXTURES},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  PG_MAX_BINDLESS_TEXTURES},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1                       },
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2                       },
        {VK_DESCRIPTOR_TYPE_SAMPLER,        12                      },
    };
    s_descriptorAllocator.Init( 1, poolSizes, "global" );

    s_globalDescriptorSet = s_descriptorAllocator.Allocate( s_globalDescriptorSetLayout, "global" );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )
}

void FreeGlobalDescriptorData()
{
    vkDestroyDescriptorSetLayout( rg.device, s_globalDescriptorSetLayout, nullptr );
    DEBUG_BUILD_ONLY( s_globalDescriptorSetLayout = VK_NULL_HANDLE );
#if USING( PG_DESCRIPTOR_BUFFER )
    s_globalDescriptorBuffer.Free();
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    DEBUG_BUILD_ONLY( s_globalDescriptorSet = VK_NULL_HANDLE );
    s_descriptorAllocator.Free();
#endif // #else  // #if USING( PG_DESCRIPTOR_BUFFER )
}

const VkDescriptorSetLayout& GetGlobalDescriptorSetLayout() { return s_globalDescriptorSetLayout; }
#if USING( PG_DESCRIPTOR_BUFFER )
DescriptorBuffer& GetGlobalDescriptorBuffer() { return s_globalDescriptorBuffer; }
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
const VkDescriptorSet& GetGlobalDescriptorSet() { return s_globalDescriptorSet; }
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

} // namespace PG::Gfx
