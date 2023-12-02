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

struct SamplerDescriptor
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

private:
    SamplerDescriptor m_desc;
    VkSampler m_handle = VK_NULL_HANDLE;
    VkDevice m_device  = VK_NULL_HANDLE;
};

void InitSamplers();
void FreeSamplers();
Gfx::Sampler* AddSampler( Gfx::SamplerDescriptor& desc );
Gfx::Sampler* GetSampler( const std::string& name );

} // namespace PG::Gfx
