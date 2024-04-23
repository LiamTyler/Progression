#include "renderer/graphics_api/swapchain.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "vk-bootstrap/VkBootstrap.h"
#include <vector>

namespace PG::Gfx
{

bool Swapchain::Create( uint32_t width, uint32_t height )
{
    m_device = rg.device.GetHandle();
    vkb::SwapchainBuilder swapchainBuilder{ rg.physicalDevice.GetHandle(), rg.device.GetHandle(), rg.surface };

    swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format( VkSurfaceFormatKHR{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } )
        // use vsync present mode
        .set_desired_present_mode( VK_PRESENT_MODE_FIFO_KHR )
        .set_desired_extent( width, height )
        .add_image_usage_flags(
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT );

    auto swap_ret = swapchainBuilder.build();
    if ( !swap_ret )
    {
        LOG_ERR( "Failed to create swapchain. Error: %s", swap_ret.error().message().c_str() );
        return false;
    }

    vkb::Swapchain vkbSwapchain = swap_ret.value();

    m_width      = vkbSwapchain.extent.width;
    m_height     = vkbSwapchain.extent.height;
    m_handle     = vkbSwapchain.swapchain;
    m_images     = vkbSwapchain.get_images().value();
    m_imageViews = vkbSwapchain.get_image_views().value();
    for ( size_t i = 0; i < m_images.size(); ++i )
    {
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( m_imageViews[i], "swapchain " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( m_images[i], "swapchain " + std::to_string( i ) );
    }

    return true;
}

uint32_t Swapchain::AcquireNextImage( const Semaphore& presentCompleteSemaphore )
{
    VK_CHECK_RESULT(
        vkAcquireNextImageKHR( m_device, m_handle, UINT64_MAX, presentCompleteSemaphore.GetHandle(), VK_NULL_HANDLE, &m_currentImageIdx ) );
    return m_currentImageIdx;
}

void Swapchain::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    for ( size_t i = 0; i < m_imageViews.size(); ++i )
    {
        vkDestroyImageView( m_device, m_imageViews[i], nullptr );
    }

    vkDestroySwapchainKHR( m_device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

Swapchain::operator bool() const { return m_handle != VK_NULL_HANDLE; }
uint32_t Swapchain::GetCurrentImageIndex() const { return m_currentImageIdx; }
PixelFormat Swapchain::GetFormat() const { return VulkanToPGPixelFormat( m_imageFormat ); }
uint32_t Swapchain::GetWidth() const { return m_width; }
uint32_t Swapchain::GetHeight() const { return m_height; }
VkSwapchainKHR Swapchain::GetHandle() const { return m_handle; }
uint32_t Swapchain::GetNumImages() const { return (uint32_t)m_images.size(); }
VkImage Swapchain::GetImage() const { return m_images[m_currentImageIdx]; }
VkImageView Swapchain::GetImageView() const { return m_imageViews[m_currentImageIdx]; }
VkImage Swapchain::GetImageAt( uint32_t index ) const { return m_images[index]; }
VkImageView Swapchain::GetImageViewAt( uint32_t index ) const { return m_imageViews[index]; }

} // namespace PG::Gfx
