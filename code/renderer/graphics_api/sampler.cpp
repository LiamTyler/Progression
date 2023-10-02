#include "renderer/graphics_api/sampler.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include <unordered_map>

namespace PG
{
namespace Gfx
{

    static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

    Gfx::Sampler* AddSampler( Gfx::SamplerDescriptor& desc )
    {
        auto it = s_samplers.find( desc.name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        else
        {
            s_samplers[desc.name] = rg.device.NewSampler( desc );
            return &s_samplers[desc.name];
        }
    }


    Gfx::Sampler* GetSampler( const std::string& name )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }

        return nullptr;
    }


    // helper for adding the 4 wrap variants, 2 in U and 2 in V
    static void AddWrapSamplers( const SamplerDescriptor& samplerTemplate )
    {
        SamplerDescriptor samplerDesc = samplerTemplate;

        samplerDesc.name = samplerTemplate.name + "clampU_clampV";
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name = samplerTemplate.name + "clampU_wrapV";
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name = samplerTemplate.name + "wrapU_clampV";
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name = samplerTemplate.name + "wrapU_wrapV";
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
        AddSampler( samplerDesc );
    }


    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;
        samplerDesc.maxAnisotropy = 16.0f;

        samplerDesc.name      = "nearest_";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        AddWrapSamplers( samplerDesc );

        samplerDesc.name      = "bilinear_";
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        AddWrapSamplers( samplerDesc );

        samplerDesc.name      = "trilinear_";
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.mipFilter = MipFilterMode::LINEAR;
        AddWrapSamplers( samplerDesc );

        samplerDesc.name        = "shadow_map";
        samplerDesc.minFilter   = FilterMode::NEAREST;
        samplerDesc.magFilter   = FilterMode::NEAREST;
        samplerDesc.mipFilter   = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeV   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeW   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.borderColor = BorderColor::OPAQUE_WHITE_FLOAT;
        AddSampler( samplerDesc );
    }


    void FreeSamplers()
    {
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler.Free();
        }
    }


    void Sampler::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroySampler( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    Sampler::operator bool() const { return m_handle != VK_NULL_HANDLE; }
    std::string Sampler::GetName() const { return m_desc.name; }
    FilterMode Sampler::GetMinFilter() const { return m_desc.minFilter; }
    FilterMode Sampler::GetMagFilter() const { return m_desc.magFilter; }
    WrapMode Sampler::GetWrapModeU() const { return m_desc.wrapModeU; }
    WrapMode Sampler::GetWrapModeV() const { return m_desc.wrapModeV; }
    WrapMode Sampler::GetWrapModeW() const { return m_desc.wrapModeW; }
    float Sampler::GetMaxAnisotropy() const { return m_desc.maxAnisotropy; }
    BorderColor Sampler::GetBorderColor() const { return m_desc.borderColor; }
    VkSampler Sampler::GetHandle() const { return m_handle; }

} // namespace Gfx
} // namespace PG
