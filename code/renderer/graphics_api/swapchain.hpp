#pragma once

#include "renderer/graphics_api/synchronization.hpp"
#include "renderer/vulkan.hpp"
#include <array>
#include <string>

#define GFX_MAX_SWAPCHAIN_IMAGES 3

namespace PG
{
namespace Gfx
{

class SwapChain
{
public:

    bool Create( VkDevice device, uint32_t preferredWidth, uint32_t preferredHeight );
    uint32_t AcquireNextImage( const Semaphore& presentCompleteSemaphore );
    void Free();
    operator bool() const;

    uint32_t GetCurrentImageIndex() const;
    VkFormat GetFormat() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    VkSwapchainKHR GetHandle() const;
    uint32_t GetNumImages() const;
    VkImage GetImage( uint32_t index ) const;
    VkImageView GetImageView( uint32_t index ) const;

private:
    VkDevice m_device;
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    VkFormat m_imageFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_currentImage;
    uint32_t m_numImages;
    std::array<VkImage, GFX_MAX_SWAPCHAIN_IMAGES> m_images;
    std::array<VkImageView, GFX_MAX_SWAPCHAIN_IMAGES> m_imageViews;
};

} // namespace Gfx
} // namespace PG
