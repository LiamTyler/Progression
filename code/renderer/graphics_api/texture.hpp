#pragma once

#include "c_shared/bindless.h"
#include "core/image_types.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/sampler.hpp"

namespace PG::Gfx
{

struct TextureCreateInfo
{
    ImageType type       = ImageType::TYPE_2D;
    PixelFormat format   = PixelFormat::NUM_PIXEL_FORMATS;
    SamplerType sampler  = SAMPLER_TRILINEAR_WRAP_U_WRAP_V;
    uint8_t mipLevels    = 1;
    uint16_t arrayLayers = 1;
    uint16_t width       = 0;
    uint16_t height      = 0;
    uint16_t depth       = 1;
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
    uint16_t GetBindlessIndex() const;
    size_t GetTotalBytes() const;
    Sampler* GetSampler() const;
    VkImageUsageFlags GetUsage() const;
    DEBUG_BUILD_ONLY( const char* GetDebugName() const { return debugName; } );

    operator bool() const;

private:
    TextureCreateInfo m_info;
    VkImage m_image            = VK_NULL_HANDLE;
    VkImageView m_imageView    = VK_NULL_HANDLE;
    VmaAllocation m_allocation = nullptr;
    Sampler* m_sampler         = nullptr;
    uint16_t m_bindlessIndex   = PG_INVALID_TEXTURE_INDEX;
    DEBUG_BUILD_ONLY( char* debugName = nullptr );
};

enum class ImageLayout
{
    UNDEFINED                          = 0,
    GENERAL                            = 1,
    COLOR_ATTACHMENT                   = 2,
    DEPTH_STENCIL_ATTACHMENT           = 3,
    DEPTH_STENCIL_READ_ONLY            = 4,
    SHADER_READ_ONLY                   = 5,
    TRANSFER_SRC                       = 6,
    TRANSFER_DST                       = 7,
    PREINITIALIZED                     = 8,
    DEPTH_READ_ONLY_STENCIL_ATTACHMENT = 1000117000,
    DEPTH_ATTACHMENT_STENCIL_READ_ONLY = 1000117001,
    DEPTH_ATTACHMENT                   = 1000241000,
    DEPTH_READ_ONLY                    = 1000241001,
    STENCIL_ATTACHMENT                 = 1000241002,
    STENCIL_READ_ONLY                  = 1000241003,
    READ_ONLY                          = 1000314000,
    ATTACHMENT                         = 1000314001,
    PRESENT_SRC_KHR                    = 1000001002,

    NUM_IMAGE_LAYOUT = 18
};

bool ImageLayoutHasDepthAspect( ImageLayout layout );
bool ImageLayoutHasStencilAspect( ImageLayout layout );
std::string ImageLayoutToString( ImageLayout layout );

} // namespace PG::Gfx
