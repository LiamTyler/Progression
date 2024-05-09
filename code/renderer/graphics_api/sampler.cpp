#include "renderer/graphics_api/sampler.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include <unordered_map>

namespace PG::Gfx
{

static Sampler s_builtInSamplers[NUM_SAMPLERS];
static std::unordered_map<std::string, Sampler> s_customSamplers;

Sampler* GetSampler( SamplerType samplerType ) { return &s_builtInSamplers[samplerType]; }

Sampler* AddCustomSampler( SamplerDescriptor& desc )
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
static void AddWrapSamplers( SamplerType type, const SamplerDescriptor& samplerTemplate )
{
    SamplerDescriptor samplerDesc = samplerTemplate;

    samplerDesc.name      = samplerTemplate.name + "clampU_clampV";
    samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
    // s_builtInSamplers[type] = rg.device.NewSampler( samplerDesc );

    samplerDesc.name      = samplerTemplate.name + "wrapU_clampV";
    samplerDesc.wrapModeU = WrapMode::REPEAT;
    samplerDesc.wrapModeV = WrapMode::REPEAT;
    samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;

    samplerDesc.name      = samplerTemplate.name + "clampU_wrapV";
    samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
    samplerDesc.wrapModeV = WrapMode::REPEAT;
    samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;

    samplerDesc.name      = samplerTemplate.name + "wrapU_wrapV";
    samplerDesc.wrapModeU = WrapMode::REPEAT;
    samplerDesc.wrapModeV = WrapMode::REPEAT;
    samplerDesc.wrapModeW = WrapMode::REPEAT;
}

void InitSamplers()
{
    SamplerDescriptor samplerDesc;
    samplerDesc.maxAnisotropy = 16.0f;

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
}

void FreeSamplers()
{
    for ( int i = 0; i < NUM_SAMPLERS; ++i )
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

std::string Sampler::GetName() const { return m_desc.name; }
FilterMode Sampler::GetMinFilter() const { return m_desc.minFilter; }
FilterMode Sampler::GetMagFilter() const { return m_desc.magFilter; }
WrapMode Sampler::GetWrapModeU() const { return m_desc.wrapModeU; }
WrapMode Sampler::GetWrapModeV() const { return m_desc.wrapModeV; }
WrapMode Sampler::GetWrapModeW() const { return m_desc.wrapModeW; }
float Sampler::GetMaxAnisotropy() const { return m_desc.maxAnisotropy; }
BorderColor Sampler::GetBorderColor() const { return m_desc.borderColor; }
VkSampler Sampler::GetHandle() const { return m_handle; }
Sampler::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Sampler::operator VkSampler() const { return m_handle; }

} // namespace PG::Gfx
