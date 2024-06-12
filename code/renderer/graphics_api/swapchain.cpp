#include "renderer/graphics_api/swapchain.hpp"
#include "core/cpu_profiling.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "vk-bootstrap/VkBootstrap.h"
#include <vector>

namespace PG::Gfx
{

bool Swapchain::Create( u32 width, u32 height )
{
    PGP_ZONE_SCOPEDN( "Swapchain::Create" );
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
    if ( m_width != width && m_height != height )
        LOG_WARN( "Swapchain requested size (%u x %u) actual size: (%u x %u )", width, height, m_width, m_height );

    TextureCreateInfo texInfo( m_imageFormat, m_width, m_height );
    texInfo.sampler = SAMPLER_BILINEAR;

    // PG_ASSERT( m_width == width && m_height == height, "Later textures (r_globals) are allocated assuming this" );
    m_handle                            = vkbSwapchain.swapchain;
    std::vector<VkImage> images         = vkbSwapchain.get_images().value();
    std::vector<VkImageView> imageViews = vkbSwapchain.get_image_views().value();
    m_textures.resize( images.size() );
    for ( size_t i = 0; i < images.size(); ++i )
    {
        Texture& tex = m_textures[i];
#if USING( DEBUG_BUILD )
        std::string name = "swapchainTex_" + std::to_string( i );
        tex.debugName    = (char*)malloc( name.length() + 1 );
        strcpy( tex.debugName, name.c_str() );
#endif // #if USING( DEBUG_BUILD )
        tex.m_info          = texInfo;
        tex.m_image         = images[i];
        tex.m_imageView     = imageViews[i];
        tex.m_allocation    = nullptr;
        tex.m_sampler       = GetSampler( SAMPLER_BILINEAR );
        tex.m_bindlessIndex = BindlessManager::AddTexture( &tex );
        PG_DEBUG_MARKER_SET_IMAGE_NAME( images[i], "swapchain " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( imageViews[i], "swapchain " + std::to_string( i ) );
    }

    return true;
}

bool Swapchain::Recreate( u32 preferredWidth, u32 preferredHeight )
{
    rg.device.WaitForIdle();
    Free();
    return Create( preferredWidth, preferredHeight );
}

bool Swapchain::AcquireNextImage( const Semaphore& presentCompleteSemaphore )
{
    PGP_ZONE_SCOPEDN( "Swapchain::AcquireNextImage" );
    VkResult res = vkAcquireNextImageKHR( rg.device, m_handle, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &m_currentImageIdx );
    bool resizeNeeded = res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR;
    PG_ASSERT( res == VK_SUCCESS || resizeNeeded, "vkAcquireNextImageKHR failed with error %d", res );
    return !resizeNeeded;
}

void Swapchain::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    for ( size_t i = 0; i < m_textures.size(); ++i )
    {
        vkDestroyImageView( rg.device, m_textures[i].GetView(), nullptr );
        BindlessManager::RemoveTexture( m_textures[i].m_bindlessIndex );
    }
    m_textures.clear();

    vkDestroySwapchainKHR( rg.device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

Swapchain::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Swapchain::operator VkSwapchainKHR() const { return m_handle; }
u32 Swapchain::GetCurrentImageIndex() const { return m_currentImageIdx; }
PixelFormat Swapchain::GetFormat() const { return m_imageFormat; }
u32 Swapchain::GetWidth() const { return m_width; }
u32 Swapchain::GetHeight() const { return m_height; }
VkSwapchainKHR Swapchain::GetHandle() const { return m_handle; }
u32 Swapchain::GetNumImages() const { return (u32)m_textures.size(); }
Texture& Swapchain::GetTexture() { return m_textures[m_currentImageIdx]; }
Texture& Swapchain::GetTextureAt( u32 index ) { return m_textures[index]; }

} // namespace PG::Gfx
