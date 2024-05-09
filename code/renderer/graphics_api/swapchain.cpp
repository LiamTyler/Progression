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
    vkb::SwapchainBuilder swapchainBuilder{ rg.physicalDevice, rg.device, rg.surface };

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

    m_imageFormat = VulkanToPGPixelFormat( vkbSwapchain.image_format );
    m_width       = vkbSwapchain.extent.width;
    m_height      = vkbSwapchain.extent.height;
    PG_ASSERT( m_width == width && m_height == height, "Later textures (r_globals) are allocated assuming this" );
    m_handle     = vkbSwapchain.swapchain;
    m_images     = vkbSwapchain.get_images().value();
    m_imageViews = vkbSwapchain.get_image_views().value();
    for ( size_t i = 0; i < m_images.size(); ++i )
    {
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( m_imageViews[i], "swapchain " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_NAME( m_images[i], "swapchain " + std::to_string( i ) );
    }

    return true;
}

bool Swapchain::Recreate( uint32_t preferredWidth, uint32_t preferredHeight )
{
    rg.device.WaitForIdle();
    Free();
    return Create( preferredWidth, preferredHeight );
}

bool Swapchain::AcquireNextImage( const Semaphore& presentCompleteSemaphore )
{
    VkResult res = vkAcquireNextImageKHR( rg.device, m_handle, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &m_currentImageIdx );
    bool resizeNeeded = res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR;
    PG_ASSERT( res == VK_SUCCESS || resizeNeeded, "vkAcquireNextImageKHR failed with error %d", res );
    return !resizeNeeded;
}

void Swapchain::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    for ( size_t i = 0; i < m_imageViews.size(); ++i )
    {
        vkDestroyImageView( rg.device, m_imageViews[i], nullptr );
    }

    vkDestroySwapchainKHR( rg.device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

Swapchain::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Swapchain::operator VkSwapchainKHR() const { return m_handle; }
uint32_t Swapchain::GetCurrentImageIndex() const { return m_currentImageIdx; }
PixelFormat Swapchain::GetFormat() const { return m_imageFormat; }
uint32_t Swapchain::GetWidth() const { return m_width; }
uint32_t Swapchain::GetHeight() const { return m_height; }
VkSwapchainKHR Swapchain::GetHandle() const { return m_handle; }
uint32_t Swapchain::GetNumImages() const { return (uint32_t)m_images.size(); }
VkImage Swapchain::GetImage() const { return m_images[m_currentImageIdx]; }
VkImageView Swapchain::GetImageView() const { return m_imageViews[m_currentImageIdx]; }
VkImage Swapchain::GetImageAt( uint32_t index ) const { return m_images[index]; }
VkImageView Swapchain::GetImageViewAt( uint32_t index ) const { return m_imageViews[index]; }

} // namespace PG::Gfx
