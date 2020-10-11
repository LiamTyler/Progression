#pragma once

#include "renderer/vulkan.hpp"
#include <string>


namespace PG
{
namespace Gfx
{

    enum class FilterMode
    {
        NEAREST = 0,
        LINEAR  = 1,
    };

    enum class MipFilterMode
    {
        NEAREST = 0,
        LINEAR  = 1,
    };

    enum class WrapMode
    {
        REPEAT          = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE   = 2,
        CLAMP_TO_BORDER = 3,
    };

    enum class BorderColor
    {
        TRANSPARENT_BLACK_FLOAT = 0,
        TRANSPARENT_BLACK_INT   = 1,
        OPAQUE_BLACK_FLOAT      = 2,
        OPAQUE_BLACK_INT        = 3,
        OPAQUE_WHITE_FLOAT      = 4,
        OPAQUE_WHITE_INT        = 5,
    };

    class SamplerDescriptor
    {
    public:
        std::string name        = "default";
        FilterMode minFilter    = FilterMode::LINEAR;
        FilterMode magFilter    = FilterMode::LINEAR;
        MipFilterMode mipFilter = MipFilterMode::LINEAR;
        WrapMode wrapModeU      = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeV      = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeW      = WrapMode::CLAMP_TO_EDGE;
        float maxAnisotropy     = 1.0f;
        BorderColor borderColor = BorderColor::OPAQUE_BLACK_INT;
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

} // namespace Gfx
} // namespace PG
