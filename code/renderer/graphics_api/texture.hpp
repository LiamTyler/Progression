#pragma once

#include "core/image_types.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/vulkan.hpp"
#include <string>

#define PG_INVALID_TEXTURE_INDEX 65535 // keep in sync with c_shared/defines.h

namespace PG::Gfx
{

class Sampler;

struct TextureDescriptor
{
    ImageType type       = ImageType::TYPE_2D;
    PixelFormat format   = PixelFormat::NUM_PIXEL_FORMATS;
    uint32_t mipLevels   = 1;
    uint32_t arrayLayers = 1;
    uint32_t width       = 0;
    uint32_t height      = 0;
    uint32_t depth       = 1;
    VkImageUsageFlags usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    std::string sampler     = "trilinear_wrapU_wrapV";
    bool addToBindlessArray = true;

    size_t TotalSizeInBytes() const;
};

class Texture
{
    friend class Device;
    friend class RenderGraph;
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
    VkImage GetHandle() const;
    VkImageView GetView() const;
    VkDeviceMemory GetMemoryHandle() const;
    uint16_t GetBindlessArrayIndex() const;
    size_t GetTotalBytes() const;
    Sampler* GetSampler() const;

    operator bool() const;

private:
    TextureDescriptor m_desc;
    VkImage m_image               = VK_NULL_HANDLE;
    VkImageView m_imageView       = VK_NULL_HANDLE;
    VkDeviceMemory m_memory       = VK_NULL_HANDLE;
    VkDevice m_device             = VK_NULL_HANDLE;
    Sampler* m_sampler            = nullptr;
    uint16_t m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;
};

} // namespace PG::Gfx
