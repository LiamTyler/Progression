#include "renderer/r_texture_manager.hpp"
#include "renderer/graphics_api/sampler.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/render_system.hpp"
#include "renderer/vulkan.hpp"
#include "shaders/c_shared/defines.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <bitset>
#include <deque>

struct PendingFree
{
    uint16_t slot;
    uint8_t pendingFor; // how many frames this has been pending
};

static uint16_t s_currentSlot;
static std::bitset<PG_MAX_BINDLESS_TEXTURES> s_slotsInUse;
static std::vector<uint16_t> s_freeSlots;
static std::vector<PendingFree> s_pendingFrees;
static std::vector<VkWriteDescriptorSet> s_setWrites;
static std::vector<VkDescriptorImageInfo> s_imageInfos;
struct TexInfo
{
    VkImageView view;
    VkSampler sampler;
};

static std::bitset<PG_MAX_BINDLESS_TEXTURES> s_pendingAddBitset;
static std::vector<std::pair<uint16_t, TexInfo>> s_pendingAdds;

namespace PG::Gfx::TextureManager
{

void Init()
{
    Shutdown();
    s_setWrites.reserve( 256 );
    s_imageInfos.reserve( 256 );
    s_imageInfos.reserve( 256 );
    s_pendingAdds.reserve( 256 );
    s_freeSlots.reserve( 256 );
}

void Shutdown()
{
    s_setWrites.clear();
    s_imageInfos.clear();
    s_pendingAdds.clear();
    s_slotsInUse.reset();
    s_pendingAddBitset.reset();
    s_currentSlot = 0;
    s_freeSlots.clear();
    s_pendingFrees.clear();
}

uint16_t GetOpenSlot( Texture* tex )
{
    uint16_t openSlot;
    if ( s_freeSlots.empty() )
    {
        openSlot = s_currentSlot;
        PG_ASSERT( s_currentSlot < 0xFFFF );
        ++s_currentSlot;
    }
    else
    {
        openSlot = s_freeSlots.back();
        s_freeSlots.pop_back();
    }
    PG_ASSERT( openSlot < PG_MAX_BINDLESS_TEXTURES && !s_slotsInUse[openSlot] );
    s_slotsInUse[openSlot]       = true;
    s_pendingAddBitset[openSlot] = true;
    TexInfo info                 = { tex->GetView(), tex->GetSampler()->GetHandle() };
    s_pendingAdds.emplace_back( openSlot, info );
    return openSlot;
}

void FreeSlot( uint16_t slot )
{
    PG_ASSERT( s_slotsInUse[slot] );
    s_slotsInUse[slot] = false;
    if ( s_pendingAddBitset[slot] )
    {
        size_t numPending = s_pendingAdds.size();
        for ( size_t i = 0; i < numPending; ++i )
        {
            if ( s_pendingAdds[i].first == slot )
            {
                std::swap( s_pendingAdds[i], s_pendingAdds[numPending - 1] );
                s_pendingAdds.pop_back();
                break;
            }
        }
        s_freeSlots.push_back( slot );
        s_pendingAddBitset[slot] = false;
    }
    else
    {
        s_pendingFrees.push_back( { slot, 0 } );
    }
}

// With multiple frames in flight at once, we can't actually free a slot until we know it's no longer being used,
// so we wait until that many frames have gone by
static void ProcessPendingFrees()
{
    if ( s_pendingFrees.empty() )
        return;

    size_t slotsToFree = 0;
    while ( slotsToFree < s_pendingFrees.size() && s_pendingFrees[slotsToFree].pendingFor < NUM_FRAME_OVERLAP )
        ++slotsToFree;

    if ( slotsToFree == 0 )
        return;

    for ( size_t i = 0; i < slotsToFree; ++i )
    {
        s_freeSlots.push_back( s_pendingFrees[i].slot );
    }
    s_pendingFrees.erase( s_pendingFrees.begin(), s_pendingFrees.begin() + slotsToFree );

    for ( size_t i = 0; i < s_pendingFrees.size(); ++i )
    {
        ++s_pendingFrees[i].pendingFor;
    }
}

void UpdateDescriptors( const DescriptorSet& textureDescriptorSet )
{
    ProcessPendingFrees();

    if ( !s_pendingAdds.empty() )
    {
        s_setWrites.resize( s_pendingAdds.size(), {} );
        s_imageInfos.resize( s_pendingAdds.size() );
        for ( size_t i = 0; i < s_setWrites.size(); ++i )
        {
            s_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            s_imageInfos[i].imageView   = s_pendingAdds[i].second.view;
            s_imageInfos[i].sampler     = s_pendingAdds[i].second.sampler;
            uint32_t slot               = s_pendingAdds[i].first;

            s_setWrites[i] =
                WriteDescriptorSet_Image( textureDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &s_imageInfos[i], 1, slot );
            s_pendingAddBitset[slot] = false;
        }

        rg.device.UpdateDescriptorSets( static_cast<uint32_t>( s_setWrites.size() ), s_setWrites.data() );
        s_pendingAdds.clear();
    }
}

} // namespace PG::Gfx::TextureManager
