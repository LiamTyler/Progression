#include "renderer/r_texture_manager.hpp"
#include "renderer/graphics_api/sampler.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/vulkan.hpp"
#include "shaders/c_shared/defines.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <bitset>
#include <deque>

static uint16_t s_currentSlot;
static std::bitset< PG_MAX_BINDLESS_TEXTURES > s_slotsInUse;
static std::deque< uint16_t > s_freeSlots;
static std::vector< VkWriteDescriptorSet > s_setWrites;
static std::vector< VkDescriptorImageInfo > s_imageInfos;
struct TexInfo
{
    VkImageView view;
    VkSampler sampler;
};

static std::bitset< PG_MAX_BINDLESS_TEXTURES > s_pendingSlots;
static std::vector< std::pair< uint16_t, TexInfo > > s_pendingUpdates;

namespace PG
{
namespace Gfx
{
namespace TextureManager
{
    void Init()
    {
        Shutdown();
        s_setWrites.reserve( 256 );
        s_imageInfos.reserve( 256 );
        s_imageInfos.reserve( 256 );
        s_pendingUpdates.reserve( 256 );
    }

    void Shutdown()
    {
        s_setWrites.clear();
        s_imageInfos.clear();
        s_pendingUpdates.clear();
        s_slotsInUse.reset();
        s_pendingSlots.reset();
        s_currentSlot = 0;
        s_freeSlots.clear();
    }

    uint16_t GetOpenSlot( Texture* tex )
    {
        uint16_t openSlot;
        if ( s_freeSlots.empty() )
        {
            openSlot = s_currentSlot;
            ++s_currentSlot;
        }
        else
        {
            openSlot = s_freeSlots.back();
            s_freeSlots.pop_back();
        }
        PG_ASSERT( openSlot < PG_MAX_BINDLESS_TEXTURES && !s_slotsInUse[openSlot] );
        s_slotsInUse[openSlot] = true;
        s_pendingSlots[openSlot] = true;
        TexInfo info = { tex->GetView(), tex->GetSampler()->GetHandle() };
        s_pendingUpdates.emplace_back( openSlot, info );
        return openSlot;
    }

    void FreeSlot( uint16_t slot )
    {
        PG_ASSERT( s_slotsInUse[slot] );
        s_slotsInUse[slot] = false;
        s_freeSlots.push_front( slot );
        if ( s_pendingSlots[slot] )
        {
            size_t numPending = s_pendingUpdates.size();
            for ( size_t i = 0; i < numPending; ++i )
            {
                if ( s_pendingUpdates[i].first == slot )
                {
                    std::swap( s_pendingUpdates[i], s_pendingUpdates[numPending - 1] );
                    s_pendingUpdates.pop_back();
                }
            }
            s_pendingSlots[slot] = false;
        }
    }

    void UpdateSampler( Texture* tex )
    {
        PG_ASSERT( tex && tex->GetBindlessArrayIndex() != PG_INVALID_TEXTURE_INDEX && s_slotsInUse[tex->GetBindlessArrayIndex()] );
        TexInfo info = { tex->GetView(), tex->GetSampler()->GetHandle() };
        s_pendingUpdates.emplace_back( tex->GetBindlessArrayIndex(), info );
    }

    void UpdateDescriptors( const DescriptorSet& textureDescriptorSet )
    {
        if ( s_pendingUpdates.empty() )
        {
            return;
        }

        s_setWrites.resize( s_pendingUpdates.size(), {} );
        s_imageInfos.resize( s_pendingUpdates.size() );
        for ( size_t i = 0; i < s_setWrites.size(); ++i )
        {
            s_imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            s_imageInfos[i].imageView   = s_pendingUpdates[i].second.view;
            s_imageInfos[i].sampler     = s_pendingUpdates[i].second.sampler;
            uint32_t slot = s_pendingUpdates[i].first;

            s_setWrites[i] = WriteDescriptorSet_Image( textureDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &s_imageInfos[i], 1, slot );
            s_pendingSlots[slot] = false;
        }

        r_globals.device.UpdateDescriptorSets( static_cast< uint32_t >( s_setWrites.size() ), s_setWrites.data() );

        s_pendingUpdates.clear();
    }

} // namespace TextureManager
} // namespace Gfx
} // namespace PG
