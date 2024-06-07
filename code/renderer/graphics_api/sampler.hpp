#pragma once

#include "renderer/vulkan.hpp"
#include <string>

namespace PG::Gfx
{

enum class FilterMode : uint8_t
{
    NEAREST = 0,
    LINEAR  = 1,
};

enum class MipFilterMode : uint8_t
{
    NEAREST = 0,
    LINEAR  = 1,
};

enum class WrapMode : uint8_t
{
    REPEAT          = 0,
    MIRRORED_REPEAT = 1,
    CLAMP_TO_EDGE   = 2,
    CLAMP_TO_BORDER = 3,
};

enum class BorderColor : uint8_t
{
    TRANSPARENT_BLACK_FLOAT = 0,
    TRANSPARENT_BLACK_INT   = 1,
    OPAQUE_BLACK_FLOAT      = 2,
    OPAQUE_BLACK_INT        = 3,
    OPAQUE_WHITE_FLOAT      = 4,
    OPAQUE_WHITE_INT        = 5,
};

struct SamplerCreateInfo
{
    std::string name        = "default";
    FilterMode minFilter    = FilterMode::LINEAR;
    FilterMode magFilter    = FilterMode::LINEAR;
    MipFilterMode mipFilter = MipFilterMode::LINEAR;
    WrapMode wrapModeU      = WrapMode::CLAMP_TO_EDGE;
    WrapMode wrapModeV      = WrapMode::CLAMP_TO_EDGE;
    WrapMode wrapModeW      = WrapMode::CLAMP_TO_EDGE;
    BorderColor borderColor = BorderColor::OPAQUE_BLACK_INT;
    float maxAnisotropy     = 16.0f;
};

class Sampler
{
    friend class Device;

public:
    void Free();

    std::string GetName() const;
    FilterMode GetMinFilter() const;
    FilterMode GetMagFilter() const;
    WrapMode GetWrapModeU() const;
    WrapMode GetWrapModeV() const;
    WrapMode GetWrapModeW() const;
    float GetMaxAnisotropy() const;
    BorderColor GetBorderColor() const;
    VkSampler GetHandle() const;
    operator bool() const;
    operator VkSampler() const;

private:
    SamplerCreateInfo m_info;
    VkSampler m_handle = VK_NULL_HANDLE;
};

// Note: keep in sync with global_descriptors.glsl!
enum SamplerType : uint8_t
{
    SAMPLER_NEAREST                 = 0,
    SAMPLER_NEAREST_WRAP_U          = 1,
    SAMPLER_NEAREST_WRAP_V          = 2,
    SAMPLER_NEAREST_WRAP_U_WRAP_V   = 3,
    SAMPLER_BILINEAR                = 4,
    SAMPLER_BILINEAR_WRAP_U         = 5,
    SAMPLER_BILINEAR_WRAP_V         = 6,
    SAMPLER_BILINEAR_WRAP_U_WRAP_V  = 7,
    SAMPLER_TRILINEAR               = 8,
    SAMPLER_TRILINEAR_WRAP_U        = 9,
    SAMPLER_TRILINEAR_WRAP_V        = 10,
    SAMPLER_TRILINEAR_WRAP_U_WRAP_V = 11,

    NUM_SAMPLERS
};

void InitSamplers();
void FreeSamplers();
Sampler* GetSampler( SamplerType samplerType );
Sampler* AddCustomSampler( Gfx::SamplerCreateInfo& desc );
Sampler* GetCustomSampler( const std::string& name );

} // namespace PG::Gfx
