#pragma once

#include "shaders/c_shared/defines.h"
#include "renderer/vulkan.hpp"
#include "core/pixel_formats.hpp"
#include <string>

namespace PG
{

class Image2D;

namespace Gfx
{

    class Sampler;

    enum class ImageType : uint8_t
    {
        TYPE_1D            = 0,
        TYPE_1D_ARRAY      = 1,
        TYPE_2D            = 2,
        TYPE_2D_ARRAY      = 3,
        TYPE_CUBEMAP       = 4,
        TYPE_CUBEMAP_ARRAY = 5,
        TYPE_3D            = 6,

        NUM_IMAGE_TYPES
    };

    struct TextureDescriptor
    {
        ImageType type          = ImageType::TYPE_2D;
        PixelFormat format      = PixelFormat::NUM_PIXEL_FORMATS;
        uint32_t mipLevels      = 1;
        uint32_t arrayLayers    = 1;
        uint32_t width          = 0;
        uint32_t height         = 0;
        uint32_t depth          = 1;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        std::string sampler     = "linear_repeat_linear";
        bool addToBindlessArray = true;

        size_t TotalSizeInBytes() const;
    };


    class Texture
    {
        friend class Device;
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
        void SetSampler( Sampler* sampler );

        operator bool() const;

    private:
        TextureDescriptor m_desc;
        VkImage m_image         = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDevice m_device       = VK_NULL_HANDLE;
        Sampler* m_sampler      = nullptr;
        uint16_t m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;

    };

} // namespace Gfx
} // namespace PG
