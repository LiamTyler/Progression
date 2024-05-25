#include "renderer/r_texture_manager.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/r_globals.hpp"

using namespace PG::Gfx;
using std::vector;
using Handle = uint16_t;

struct PerFrameManagerData
{
    vector<Handle> pendingRemovals;
};

struct PendingAdd
{
    VkImageView imgView;
    Handle slot;
    TextureManager::Usage usage;
};

static PerFrameManagerData s_frameData[NUM_FRAME_OVERLAP];
static PerFrameManagerData* s_currFrameData;
static vector<Handle> s_freeSlots;
static vector<PendingAdd> s_pendingAdds;

#if USING( PG_DESCRIPTOR_BUFFER )
static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_dbProps;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
static vector<VkWriteDescriptorSet> s_scratchWrites;
static vector<VkDescriptorImageInfo> s_scratchImgInfos;
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

namespace PG::Gfx::TextureManager
{

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingRemovals.reserve( 128 );
    }
    s_currFrameData = &s_frameData[0];

    // the first slot in the bindless array we just reserve as an invalid slot
    s_freeSlots.resize( PG_MAX_BINDLESS_TEXTURES - 1 );
    for ( int i = 1; i < PG_MAX_BINDLESS_TEXTURES; ++i )
    {
        s_freeSlots[i] = PG_MAX_BINDLESS_TEXTURES - i;
    }
    static_assert( PG_INVALID_TEXTURE_INDEX == 0, "update the free slot list above if this changes" );

    s_pendingAdds.reserve( 128 );

#if USING( PG_DESCRIPTOR_BUFFER )
    s_dbProps = rg.physicalDevice.GetProperties().dbProps;
    s_descriptorBuffer.Create( s_descriptorSetLayout );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    s_scratchWrites.reserve( 2 * 128 );
    s_scratchImgInfos.reserve( 2 * 128 );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingRemovals = vector<Handle>();
    }
    s_freeSlots   = vector<Handle>();
    s_pendingAdds = vector<PendingAdd>();
}

void Update()
{
    s_freeSlots.insert( s_freeSlots.end(), s_currFrameData->pendingRemovals.begin(), s_currFrameData->pendingRemovals.end() );
    s_currFrameData->pendingRemovals.clear();
    s_currFrameData = &s_frameData[rg.currentFrameIdx];

    uint32_t numAdds = static_cast<uint32_t>( s_pendingAdds.size() );
    if ( !numAdds )
        return;

#if USING( PG_DESCRIPTOR_BUFFER )
    DescriptorBuffer& descriptorBuffer = GetGlobalDescriptorBuffer();
    char* dbPtr                        = descriptorBuffer.buffer.GetMappedPtr();
    for ( uint32_t i = 0; i < numAdds; ++i )
    {
        const PendingAdd& pendingAdd = s_pendingAdds[i];

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
    s_scratchWrites.reserve( 2 * numAdds );
    s_scratchImgInfos.reserve( 2 * numAdds );
    for ( uint32_t i = 0; i < numAdds; ++i )
    {
        const PendingAdd& pendingAdd = s_pendingAdds[i];

        if ( IsSet( pendingAdd.usage, Usage::READ ) )
        {
            VkDescriptorImageInfo& imgInfo = s_scratchImgInfos.emplace_back();
            imgInfo.sampler                = VK_NULL_HANDLE;
            imgInfo.imageView              = pendingAdd.imgView;
            imgInfo.imageLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet& write = s_scratchWrites.emplace_back( VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
            write.dstSet                = GetGlobalDescriptorSet();
            write.dstBinding            = 0;
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
            write.dstBinding            = 1;
            write.dstArrayElement       = pendingAdd.slot;
            write.descriptorCount       = 1;
            write.descriptorType        = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            write.pImageInfo            = &imgInfo;
        }
    }

    vkUpdateDescriptorSets( rg.device, numAdds, s_scratchWrites.data(), 0, nullptr );

    s_scratchWrites.clear();
    s_scratchImgInfos.clear();
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
    s_pendingAdds.clear();
}

Handle AddTexture( VkImageView imgView, Usage usage )
{
    PG_ASSERT( usage != Usage::NONE );
    PG_ASSERT( !s_freeSlots.empty(), "No more slots in the bindless array!" );
    Handle openSlot = s_freeSlots.back();
    s_freeSlots.pop_back();
    s_pendingAdds.emplace_back( imgView, openSlot, usage );

    return openSlot;
}

void RemoveTexture( Handle index ) { s_currFrameData->pendingRemovals.push_back( index ); }

} // namespace PG::Gfx::TextureManager
