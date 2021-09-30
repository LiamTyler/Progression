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
            s_samplers[desc.name] = r_globals.device.NewSampler( desc );
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


    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.name      = "nearest_clamped_nearest";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name      = "nearest_repeat_nearest";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
        AddSampler( samplerDesc );

        samplerDesc.name        = "shadow_map";
        samplerDesc.minFilter   = FilterMode::NEAREST;
        samplerDesc.magFilter   = FilterMode::NEAREST;
        samplerDesc.mipFilter   = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeV   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeW   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.borderColor = BorderColor::OPAQUE_WHITE_FLOAT;
        AddSampler( samplerDesc );

        samplerDesc.name        = "linear_clamped_linear";
        samplerDesc.minFilter   = FilterMode::LINEAR;
        samplerDesc.magFilter   = FilterMode::LINEAR;
        samplerDesc.mipFilter   = MipFilterMode::LINEAR;
        samplerDesc.wrapModeU   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.borderColor = BorderColor::OPAQUE_WHITE_FLOAT;
        AddSampler( samplerDesc );

        samplerDesc.name      = "linear_repeat_linear";
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.mipFilter = MipFilterMode::LINEAR;
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
        samplerDesc.maxAnisotropy = 16.0f;
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
