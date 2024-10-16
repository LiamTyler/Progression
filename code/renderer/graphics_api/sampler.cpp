#include "renderer/graphics_api/sampler.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include <unordered_map>

namespace PG::Gfx
{

static Sampler s_builtInSamplers[NUM_SAMPLERS];
static std::unordered_map<std::string, Sampler> s_customSamplers;

Sampler* GetSampler( SamplerType samplerType ) { return &s_builtInSamplers[samplerType]; }

Sampler* AddCustomSampler( SamplerCreateInfo& desc )
{
    auto it = s_customSamplers.find( desc.name );
    if ( it != s_customSamplers.end() )
    {
        return &it->second;
    }
    else
    {
        s_customSamplers[desc.name] = rg.device.NewSampler( desc );
        return &s_customSamplers[desc.name];
    }
}

Sampler* GetCustomSampler( const std::string& name )
{
    auto it = s_customSamplers.find( name );
    if ( it != s_customSamplers.end() )
    {
        return &it->second;
    }

    return nullptr;
}

// helper for adding the 4 wrap variants, 2 in U and 2 in V
static void AddWrapSamplers( SamplerType type, const SamplerCreateInfo& samplerTemplate )
{
    SamplerCreateInfo samplerDesc = samplerTemplate;

    samplerDesc.name                          = samplerTemplate.name + "clampU_clampV";
    samplerDesc.wrapModeU                     = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeV                     = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeW                     = WrapMode::CLAMP_TO_EDGE;
    s_builtInSamplers[Underlying( type ) + 0] = rg.device.NewSampler( samplerDesc );

    samplerDesc.name                          = samplerTemplate.name + "wrapU_clampV";
    samplerDesc.wrapModeU                     = WrapMode::REPEAT;
    samplerDesc.wrapModeV                     = WrapMode::REPEAT;
    samplerDesc.wrapModeW                     = WrapMode::CLAMP_TO_EDGE;
    s_builtInSamplers[Underlying( type ) + 1] = rg.device.NewSampler( samplerDesc );

    samplerDesc.name                          = samplerTemplate.name + "clampU_wrapV";
    samplerDesc.wrapModeU                     = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeV                     = WrapMode::REPEAT;
    samplerDesc.wrapModeW                     = WrapMode::CLAMP_TO_EDGE;
    s_builtInSamplers[Underlying( type ) + 2] = rg.device.NewSampler( samplerDesc );

    samplerDesc.name                          = samplerTemplate.name + "wrapU_wrapV";
    samplerDesc.wrapModeU                     = WrapMode::REPEAT;
    samplerDesc.wrapModeV                     = WrapMode::REPEAT;
    samplerDesc.wrapModeW                     = WrapMode::REPEAT;
    s_builtInSamplers[Underlying( type ) + 3] = rg.device.NewSampler( samplerDesc );
}

void InitSamplers()
{
    SamplerCreateInfo samplerDesc;
    samplerDesc.maxAnisotropy = rg.physicalDevice.GetProperties().maxAnisotropy;

    samplerDesc.name      = "nearest_";
    samplerDesc.minFilter = FilterMode::NEAREST;
    samplerDesc.magFilter = FilterMode::NEAREST;
    samplerDesc.mipFilter = MipFilterMode::NEAREST;
    AddWrapSamplers( SAMPLER_NEAREST, samplerDesc );

    samplerDesc.name      = "bilinear_";
    samplerDesc.minFilter = FilterMode::LINEAR;
    samplerDesc.magFilter = FilterMode::LINEAR;
    samplerDesc.mipFilter = MipFilterMode::NEAREST;
    AddWrapSamplers( SAMPLER_BILINEAR, samplerDesc );

    samplerDesc.name      = "trilinear_";
    samplerDesc.minFilter = FilterMode::LINEAR;
    samplerDesc.magFilter = FilterMode::LINEAR;
    samplerDesc.mipFilter = MipFilterMode::LINEAR;
    AddWrapSamplers( SAMPLER_TRILINEAR, samplerDesc );

    constexpr u32 NUM_SAMPLERS = Underlying( SamplerType::NUM_SAMPLERS );
    VkDescriptorImageInfo samplerInfos[NUM_SAMPLERS];
    for ( u32 i = 0; i < NUM_SAMPLERS; ++i )
    {
        samplerInfos[i]         = {};
        samplerInfos[i].sampler = s_builtInSamplers[i].GetHandle();
    }
    VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writeSet.dstSet          = GetGlobalDescriptorSet();
    writeSet.dstBinding      = PG_BINDLESS_SAMPLERS_DSET_BINDING;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = NUM_SAMPLERS;
    writeSet.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    writeSet.pImageInfo      = samplerInfos;
    vkUpdateDescriptorSets( rg.device, 1, &writeSet, 0, nullptr );
}

void FreeSamplers()
{
    for ( i32 i = 0; i < NUM_SAMPLERS; ++i )
    {
        s_builtInSamplers[i].Free();
    }
    for ( auto& [name, sampler] : s_customSamplers )
    {
        sampler.Free();
    }
}

void Sampler::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkDestroySampler( rg.device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

std::string Sampler::GetName() const { return m_info.name; }
FilterMode Sampler::GetMinFilter() const { return m_info.minFilter; }
FilterMode Sampler::GetMagFilter() const { return m_info.magFilter; }
WrapMode Sampler::GetWrapModeU() const { return m_info.wrapModeU; }
WrapMode Sampler::GetWrapModeV() const { return m_info.wrapModeV; }
WrapMode Sampler::GetWrapModeW() const { return m_info.wrapModeW; }
f32 Sampler::GetMaxAnisotropy() const { return m_info.maxAnisotropy; }
BorderColor Sampler::GetBorderColor() const { return m_info.borderColor; }
VkSampler Sampler::GetHandle() const { return m_handle; }
Sampler::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Sampler::operator VkSampler() const { return m_handle; }

} // namespace PG::Gfx
