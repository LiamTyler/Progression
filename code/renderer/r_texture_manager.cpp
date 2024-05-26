#include "renderer/r_texture_manager.hpp"
#include "c_shared/bindless.h"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/r_globals.hpp"

using namespace PG::Gfx;
using std::vector;
using TexIndex    = uint16_t;
using BufferIndex = uint16_t;

struct PerFrameManagerData
{
    vector<TexIndex> pendingTexRemovals;
    vector<BufferIndex> pendingBufferRemovals;
};

struct PendingTexAdd
{
    VkImageView imgView;
    TexIndex slot;
    BindlessManager::Usage usage;
};

struct PendingBufferAdd
{
    VkDeviceAddress bufferAddress;
    BufferIndex slot;
};

static PerFrameManagerData s_frameData[NUM_FRAME_OVERLAP];
static PerFrameManagerData* s_currFrameData;
static vector<TexIndex> s_freeTexSlots;
static vector<PendingTexAdd> s_pendingTexAdds;
static vector<BufferIndex> s_freeBufferSlots;
static vector<PendingBufferAdd> s_pendingBufferAdds;

#if USING( PG_DESCRIPTOR_BUFFER )
static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_dbProps;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
static vector<VkWriteDescriptorSet> s_scratchWrites;
static vector<VkDescriptorImageInfo> s_scratchImgInfos;
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

static Buffer s_bindlessBufferPointers;

namespace PG::Gfx::BindlessManager
{

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingTexRemovals.reserve( 128 );
        s_frameData[i].pendingBufferRemovals.reserve( 128 );
    }
    s_currFrameData = &s_frameData[0];

    // the first slot in the bindless array we just reserve as an invalid slot
    s_freeTexSlots.resize( PG_MAX_BINDLESS_TEXTURES - 1 );
    for ( int i = 1; i < PG_MAX_BINDLESS_TEXTURES; ++i )
    {
        s_freeTexSlots[i] = PG_MAX_BINDLESS_TEXTURES - i;
    }
    static_assert( PG_INVALID_TEXTURE_INDEX == 0, "update the free slot list above if this changes" );

    s_freeBufferSlots.resize( PG_MAX_BINDLESS_BUFFERS - 1 );
    for ( int i = 1; i < PG_MAX_BINDLESS_BUFFERS; ++i )
    {
        s_freeBufferSlots[i] = PG_MAX_BINDLESS_BUFFERS - i;
    }
    static_assert( PG_INVALID_BUFFER_INDEX == 0, "update the free slot list above if this changes" );

    s_pendingTexAdds.reserve( 128 );
    s_pendingBufferAdds.reserve( 128 );

#if USING( PG_DESCRIPTOR_BUFFER )
    s_dbProps = rg.physicalDevice.GetProperties().dbProps;
    s_descriptorBuffer.Create( s_descriptorSetLayout );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    s_scratchWrites.reserve( 2 * 128 );
    s_scratchImgInfos.reserve( 2 * 128 );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

    BufferCreateInfo bufInfo = {};
    bufInfo.size             = PG_MAX_BINDLESS_BUFFERS * sizeof( VkDeviceAddress );
    bufInfo.bufferUsage      = BufferUsage::STORAGE | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
    bufInfo.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    s_bindlessBufferPointers = rg.device.NewBuffer( bufInfo, "global_bindlessBufferPointers" );

    VkDescriptorBufferInfo dBufferInfo = { s_bindlessBufferPointers.GetHandle(), 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    write.dstSet          = GetGlobalDescriptorSet();
    write.dstBinding      = PG_BINDLESS_BUFFER_DSET_BINDING;
    write.descriptorCount = 1;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo     = &dBufferInfo;
    vkUpdateDescriptorSets( rg.device, 1, &write, 0, nullptr );
}

void Shutdown()
{
    s_bindlessBufferPointers.Free();
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingTexRemovals    = vector<TexIndex>();
        s_frameData[i].pendingBufferRemovals = vector<BufferIndex>();
    }
    s_freeTexSlots      = vector<TexIndex>();
    s_pendingTexAdds    = vector<PendingTexAdd>();
    s_freeBufferSlots   = vector<BufferIndex>();
    s_pendingBufferAdds = vector<PendingBufferAdd>();
}

void Update()
{
    s_freeTexSlots.insert( s_freeTexSlots.end(), s_currFrameData->pendingTexRemovals.begin(), s_currFrameData->pendingTexRemovals.end() );
    s_freeBufferSlots.insert(
        s_freeBufferSlots.end(), s_currFrameData->pendingBufferRemovals.begin(), s_currFrameData->pendingBufferRemovals.end() );
    s_currFrameData->pendingTexRemovals.clear();
    s_currFrameData->pendingBufferRemovals.clear();
    s_currFrameData = &s_frameData[rg.currentFrameIdx];

    uint32_t numTexAdds = static_cast<uint32_t>( s_pendingTexAdds.size() );
    if ( numTexAdds )
    {
#if USING( PG_DESCRIPTOR_BUFFER )
        DescriptorBuffer& descriptorBuffer = GetGlobalDescriptorBuffer();
        char* dbPtr                        = descriptorBuffer.buffer.GetMappedPtr();
        for ( uint32_t i = 0; i < numAdds; ++i )
        {
            const PendingTexAdd& pendingAdd = s_pendingTexAdds[i];

            VkDescriptorImageInfo imgInfo{};
            imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imgInfo.imageView   = pendingAdd.imgView;

            VkDescriptorGetInfoEXT descriptorInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
            descriptorInfo.type               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            descriptorInfo.data.pStorageImage = &imgInfo;
            size_t descSize                   = s_dbProps.storageImageDescriptorSize;
            vkGetDescriptorEXT( rg.device, &descriptorInfo, descSize, dbPtr + descriptorBuffer.offset + pendingAdd.slot * descSize );
        }
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
        s_scratchWrites.reserve( 2 * numTexAdds );
        s_scratchImgInfos.reserve( 2 * numTexAdds );
        for ( uint32_t i = 0; i < numTexAdds; ++i )
        {
            const PendingTexAdd& pendingAdd = s_pendingTexAdds[i];

            if ( IsSet( pendingAdd.usage, Usage::READ ) )
            {
                VkDescriptorImageInfo& imgInfo = s_scratchImgInfos.emplace_back();
                imgInfo.sampler                = VK_NULL_HANDLE;
                imgInfo.imageView              = pendingAdd.imgView;
                imgInfo.imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkWriteDescriptorSet& write = s_scratchWrites.emplace_back( VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
                write.dstSet                = GetGlobalDescriptorSet();
                write.dstBinding            = PG_BINDLESS_READ_ONLY_TEXTURE_DSET_BINDING;
                write.dstArrayElement       = pendingAdd.slot;
                write.descriptorCount       = 1;
                write.descriptorType        = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                write.pImageInfo            = &imgInfo;
            }
            if ( IsSet( pendingAdd.usage, Usage::WRITE ) )
            {
                VkDescriptorImageInfo& imgInfo = s_scratchImgInfos.emplace_back();
                imgInfo.sampler                = VK_NULL_HANDLE;
                imgInfo.imageView              = pendingAdd.imgView;
                imgInfo.imageLayout            = VK_IMAGE_LAYOUT_GENERAL;

                VkWriteDescriptorSet& write = s_scratchWrites.emplace_back( VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
                write.dstSet                = GetGlobalDescriptorSet();
                write.dstBinding            = PG_BINDLESS_RW_TEXTURE_DSET_BINDING;
                write.dstArrayElement       = pendingAdd.slot;
                write.descriptorCount       = 1;
                write.descriptorType        = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                write.pImageInfo            = &imgInfo;
            }
        }

        vkUpdateDescriptorSets( rg.device, numTexAdds, s_scratchWrites.data(), 0, nullptr );

        s_scratchWrites.clear();
        s_scratchImgInfos.clear();
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
        s_pendingTexAdds.clear();
    }

    VkDeviceAddress* bufData = reinterpret_cast<VkDeviceAddress*>( s_bindlessBufferPointers.GetMappedPtr() );
    uint32_t numBufferAdds   = static_cast<uint32_t>( s_pendingBufferAdds.size() );
    if ( numBufferAdds )
    {
        for ( const PendingBufferAdd& pendingAdd : s_pendingBufferAdds )
        {
            bufData[pendingAdd.slot] = pendingAdd.bufferAddress;
        }
        s_pendingBufferAdds.clear();
    }
}

TexIndex AddTexture( VkImageView imgView, Usage usage )
{
    PG_ASSERT( usage != Usage::NONE );
    PG_ASSERT( !s_freeTexSlots.empty(), "No more texture slots in the bindless array!" );
    TexIndex openSlot = s_freeTexSlots.back();
    s_freeTexSlots.pop_back();
    s_pendingTexAdds.emplace_back( imgView, openSlot, usage );

    return openSlot;
}

void RemoveTexture( TexIndex index )
{
    if ( index == PG_INVALID_TEXTURE_INDEX )
        return;
    s_currFrameData->pendingTexRemovals.push_back( index );
}

BufferIndex AddBuffer( const Buffer* buffer )
{
    VkDescriptorType descType = buffer->GetDescriptorType();
    if ( descType == VK_DESCRIPTOR_TYPE_MAX_ENUM )
        return PG_INVALID_BUFFER_INDEX;

    PG_ASSERT( !s_freeBufferSlots.empty(), "No more buffer slots in the bindless array!" );
    BufferIndex openSlot = s_freeBufferSlots.back();
    s_freeBufferSlots.pop_back();
    s_pendingBufferAdds.emplace_back( buffer->GetDeviceAddress(), openSlot );

    return openSlot;
}

void RemoveBuffer( BufferIndex index )
{
    if ( index == PG_INVALID_BUFFER_INDEX )
        return;
    s_currFrameData->pendingBufferRemovals.push_back( index );
}

} // namespace PG::Gfx::BindlessManager
