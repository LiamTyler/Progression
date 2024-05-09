#pragma once

#include "core/pixel_formats.hpp"
#include "renderer/graphics_api/synchronization.hpp"
#include "renderer/vulkan.hpp"
#include <array>
#include <string>

#define GFX_MAX_SWAPCHAIN_IMAGES 3

namespace PG::Gfx
{

class Swapchain
{
public:
    bool Create( uint32_t preferredWidth, uint32_t preferredHeight );
    bool Recreate( uint32_t preferredWidth, uint32_t preferredHeight );
    // returns false when a resize is needed
    bool AcquireNextImage( const Semaphore& presentCompleteSemaphore );
    void Free();
    operator bool() const;
    operator VkSwapchainKHR() const;

    uint32_t GetCurrentImageIndex() const;
    PixelFormat GetFormat() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    VkSwapchainKHR GetHandle() const;
    uint32_t GetNumImages() const;
    VkImage GetImage() const;
    VkImageView GetImageView() const;
    VkImage GetImageAt( uint32_t index ) const;
    VkImageView GetImageViewAt( uint32_t index ) const;

private:
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    PixelFormat m_imageFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_currentImageIdx;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_imageViews;
};

} // namespace PG::Gfx
