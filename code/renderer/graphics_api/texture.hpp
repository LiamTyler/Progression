#pragma once

#include "core/image_types.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/sampler.hpp"
#include "c_shared/defines.h"

namespace PG::Gfx
{

struct TextureCreateInfo
{
    ImageType type       = ImageType::TYPE_2D;
    PixelFormat format   = PixelFormat::NUM_PIXEL_FORMATS;
    SamplerType sampler  = SAMPLER_TRILINEAR_WRAP_U_WRAP_V;
    uint32_t mipLevels   = 1;
    uint32_t arrayLayers = 1;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    VkImageUsageFlags usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    TextureCreateInfo() = default;
    TextureCreateInfo( PixelFormat inFmt, uint32_t inWidth, uint32_t inHeight ) : format( inFmt ), width( inWidth ), height( inHeight ) {}
    size_t TotalSizeInBytes() const;
};

class Texture
{
    friend class Device;
    friend class TaskGraph;

public:
    Texture() = default;

    void Free();

    ImageType GetType() const;
    PixelFormat GetPixelFormat() const;
    uint32_t GetMipLevels() const;
    uint32_t GetArrayLayers() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;
    VkExtent2D GetExtent2D() const;
    VkExtent3D GetExtent3D() const;
    VkImage GetImage() const;
    VkImageView GetView() const;
    VmaAllocation GetAllocation() const;
    uint16_t GetBindlessArrayIndex() const;
    size_t GetTotalBytes() const;
    Sampler* GetSampler() const;

    operator bool() const;

private:
    TextureCreateInfo m_desc;
    VkImage m_image               = VK_NULL_HANDLE;
    VkImageView m_imageView       = VK_NULL_HANDLE;
    VmaAllocation m_allocation    = nullptr;
    VkDevice m_device             = VK_NULL_HANDLE;
    Sampler* m_sampler            = nullptr;
    uint16_t m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;
};

} // namespace PG::Gfx
