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
    Handle slot;
    VkImageView imgView;
};

static PerFrameManagerData s_frameData[NUM_FRAME_OVERLAP];
static PerFrameManagerData* s_currFrameData;
static vector<Handle> s_freeSlots;
static vector<PendingAdd> s_pendingAdds;

static VkDescriptorSetLayout s_descriptorSetLayout;
#if USING( PG_DESCRIPTOR_BUFFER )
static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_dbProps;
static DescriptorBuffer s_descriptorBuffer;
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
static DescriptorAllocator s_descriptorAllocator;
static VkDescriptorSet s_descriptorSet;
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

    DescriptorLayoutBuilder setLayoutBuilder;
    setLayoutBuilder.AddBinding( 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, PG_MAX_BINDLESS_TEXTURES );
    s_descriptorSetLayout = setLayoutBuilder.Build( VK_SHADER_STAGE_ALL, "bindless" );
#if USING( PG_DESCRIPTOR_BUFFER )
    s_dbProps = rg.physicalDevice.GetProperties().dbProps;
    s_descriptorBuffer.Create( s_descriptorSetLayout );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, PG_MAX_BINDLESS_TEXTURES},
    };
    s_descriptorAllocator.Init( 1, poolSizes, "bindless" );

    s_descriptorSet = s_descriptorAllocator.Allocate( s_descriptorSetLayout, "bindless" );
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
    vkDestroyDescriptorSetLayout( rg.device, s_descriptorSetLayout, nullptr );
    s_descriptorSetLayout = VK_NULL_HANDLE;
#if USING( PG_DESCRIPTOR_BUFFER )
    s_descriptorBuffer.Free();
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    s_descriptorSet = VK_NULL_HANDLE;
    s_descriptorAllocator.Free();
#endif // #else  // #if USING( PG_DESCRIPTOR_BUFFER )
}

const VkDescriptorSetLayout& GetBindlessSetLayout() { return s_descriptorSetLayout; }

void Update()
{
    s_freeSlots.insert( s_freeSlots.end(), s_currFrameData->pendingRemovals.begin(), s_currFrameData->pendingRemovals.end() );
    s_currFrameData->pendingRemovals.clear();
    s_currFrameData = &s_frameData[rg.currentFrameIdx % NUM_FRAME_OVERLAP];

    uint32_t numAdds = static_cast<uint32_t>( s_pendingAdds.size() );
    if ( !numAdds )
        return;

#if USING( PG_DESCRIPTOR_BUFFER )
    char* dbPtr = s_descriptorBuffer.buffer.GetMappedPtr();
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
        vkGetDescriptorEXT( rg.device, &descriptorInfo, descSize, dbPtr + s_descriptorBuffer.offset + pendingAdd.slot * descSize );
    }
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    s_scratchWrites.resize( numAdds );
    s_scratchImgInfos.resize( numAdds );
    for ( uint32_t i = 0; i < numAdds; ++i )
    {
        const PendingAdd& pendingAdd = s_pendingAdds[i];

        VkDescriptorImageInfo& imgInfo = s_scratchImgInfos[i];
        imgInfo.sampler                = VK_NULL_HANDLE;
        imgInfo.imageView              = pendingAdd.imgView;
        imgInfo.imageLayout            = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet& write = s_scratchWrites[i];
        write.sType                 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext                 = nullptr;
        write.dstSet                = s_descriptorSet;
        write.dstBinding            = 0;
        write.dstArrayElement       = pendingAdd.slot;
        write.descriptorCount       = 1;
        write.descriptorType        = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.pImageInfo            = &imgInfo;
        write.pBufferInfo           = nullptr;
        write.pTexelBufferView      = nullptr;
    }

    vkUpdateDescriptorSets( rg.device, numAdds, s_scratchWrites.data(), 0, nullptr );

    s_scratchWrites.clear();
    s_scratchImgInfos.clear();
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
    s_pendingAdds.clear();
}

Handle AddTexture( VkImageView imgView )
{
    PG_ASSERT( !s_freeSlots.empty(), "No more slots in the bindless array!" );
    Handle openSlot = s_freeSlots.back();
    s_freeSlots.pop_back();
    s_pendingAdds.emplace_back( openSlot, imgView );

    return openSlot;
}

void RemoveTexture( Handle index ) { s_currFrameData->pendingRemovals.push_back( index ); }

#if USING( PG_DESCRIPTOR_BUFFER )
const DescriptorBuffer& GetBindlessDescriptorBuffer() { return s_descriptorBuffer; }
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
const VkDescriptorSet& GetBindlessSet() { return s_descriptorSet; }
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

} // namespace PG::Gfx::TextureManager
