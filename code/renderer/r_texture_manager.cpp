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

static PerFrameManagerData s_frameData[NUM_FRAME_OVERLAP];
static PerFrameManagerData* s_currFrameData;
static vector<Handle> s_freeSlots;
static vector<VkWriteDescriptorSet> s_pendingAdds;
static vector<VkDescriptorImageInfo> s_pendingAddsImgInfo;

static DescriptorAllocator s_descriptorAllocator;
static VkDescriptorSetLayout s_bindlessSetLayout;
static VkDescriptorSet s_bindlessSet;

namespace PG::Gfx::TextureManager
{

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingRemovals.reserve( 128 );
    }
    s_currFrameData = &s_frameData[0];

    s_freeSlots.resize( PG_MAX_BINDLESS_TEXTURES );
    for ( int i = 0; i < PG_MAX_BINDLESS_TEXTURES; ++i )
    {
        s_freeSlots[i] = PG_MAX_BINDLESS_TEXTURES - i;
    }
    static_assert( PG_INVALID_TEXTURE_INDEX == 0, "update the free slot list if this changes" );

    s_pendingAdds.reserve( 128 );
    s_pendingAddsImgInfo.reserve( 128 );

    vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, PG_MAX_BINDLESS_TEXTURES}, // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    };
    s_descriptorAllocator.init_pool( rg.device, 1, poolSizes );
    DescriptorLayoutBuilder setLayoutBuilder;
    //setLayoutBuilder.mutableSupport = false;
    setLayoutBuilder.add_binding( 0, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, PG_MAX_BINDLESS_TEXTURES );
    s_bindlessSetLayout = setLayoutBuilder.build( rg.device );
    s_bindlessSet       = s_descriptorAllocator.allocate( s_bindlessSetLayout );
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_frameData[i].pendingRemovals = vector<Handle>();
    }
    s_freeSlots = vector<Handle>();
    vkDestroyDescriptorSetLayout( rg.device, s_bindlessSetLayout, nullptr );
    s_bindlessSetLayout = VK_NULL_HANDLE;
    s_bindlessSet       = VK_NULL_HANDLE;
    s_descriptorAllocator.destroy_pool();
}

const VkDescriptorSetLayout& GetBindlessSetLayout() { return s_bindlessSetLayout; }

const VkDescriptorSet& GetBindlessSet() { return s_bindlessSet; }

void Update()
{
    s_freeSlots.insert( s_freeSlots.end(), s_currFrameData->pendingRemovals.begin(), s_currFrameData->pendingRemovals.end() );
    s_currFrameData->pendingRemovals.clear();
    s_currFrameData = &s_frameData[rg.currentFrameIdx % NUM_FRAME_OVERLAP];

    uint32_t numAdds = static_cast<uint32_t>( s_pendingAdds.size() );
    if ( !numAdds )
        return;

    // can't write the pointer until now, because they'll be invalidated if s_pendingAddsImgInfo ever resizes during AddTexture
    for ( uint32_t i = 0; i < numAdds; ++i )
    {
        VkWriteDescriptorSet& add = s_pendingAdds[i];
        add.pImageInfo            = &s_pendingAddsImgInfo[i];
    }

    vkUpdateDescriptorSets( rg.device, numAdds, s_pendingAdds.data(), 0, nullptr );
    s_pendingAdds.clear();
    s_pendingAddsImgInfo.clear();
}

Handle AddTexture( VkImageView imgView )
{
    Handle openSlot = s_freeSlots.back();
    s_freeSlots.pop_back();

    VkDescriptorImageInfo imgInfo;
    imgInfo.sampler     = VK_NULL_HANDLE;
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView   = imgView;
    s_pendingAddsImgInfo.emplace_back( imgInfo );

    VkWriteDescriptorSet add{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    add.dstSet          = s_bindlessSet;
    add.dstBinding      = 0;
    add.descriptorCount = 1;
    add.dstArrayElement = openSlot;
    add.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    s_pendingAdds.emplace_back( add );

    return openSlot;
}

void RemoveTexture( Handle index ) { s_currFrameData->pendingRemovals.push_back( index ); }

} // namespace PG::Gfx::TextureManager
