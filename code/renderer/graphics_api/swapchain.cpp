#include "renderer/graphics_api/swapchain.hpp"
#include "core/cpu_profiling.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <vector>

namespace PG::Gfx
{

static VkExtent2D ChooseExtents( const VkSurfaceCapabilitiesKHR& capabilities, u32 desiredWidth, u32 desiredHeight )
{
    if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
    {
        return capabilities.currentExtent;
    }
    VkExtent2D ret;
    ret.width  = Clamp( desiredWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
    ret.height = Clamp( desiredHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

    return ret;
}

static VkSurfaceFormatKHR ChooseFormat()
{
    // BGRA8 SRGB + FIFO_KHR (vsync) is guaranteed to be available everywhere, but BGRA8 doesn't seem to
    // support VK_IMAGE_USAGE_STORAGE_BIT
    u32 formatCount;
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( rg.physicalDevice, rg.surface, &formatCount, nullptr ) );
    std::vector<VkSurfaceFormatKHR> surfaceFormats( formatCount );
    VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( rg.physicalDevice, rg.surface, &formatCount, surfaceFormats.data() ) );

    for ( u32 i = 0; i < formatCount; ++i )
    {
        VkSurfaceFormatKHR surf = surfaceFormats[i];
        if ( surf.format == VK_FORMAT_R8G8B8A8_UNORM && surf.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return surf;
        }
    }

    LOG_ERR( "TODO: ideal surface format not found" );
    return {};
}

bool Swapchain::Create( u32 width, u32 height )
{
    PGP_ZONE_SCOPEDN( "Swapchain::Create" );
    VkSurfaceCapabilitiesKHR surfaceCaps{};
    VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( rg.physicalDevice, rg.surface, &surfaceCaps ) );

    VkExtent2D extents               = ChooseExtents( surfaceCaps, width, height );
    VkSurfaceFormatKHR surfaceFormat = ChooseFormat();
    VkSwapchainCreateInfoKHR swapchainCI{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = rg.surface,
        .minImageCount    = surfaceCaps.minImageCount,
        .imageFormat      = surfaceFormat.format,
        .imageColorSpace  = surfaceFormat.colorSpace,
        .imageExtent      = extents,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform   = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode    = VK_PRESENT_MODE_FIFO_KHR // vsync
    };
    VK_CHECK( vkCreateSwapchainKHR( rg.device, &swapchainCI, nullptr, &m_handle ) );

    m_imageFormat = VulkanToPGPixelFormat( surfaceFormat.format );
    m_width       = extents.width;
    m_height      = extents.height;
    if ( m_width != width && m_height != height )
        LOG_WARN( "Swapchain requested size (%u x %u) actual size: (%u x %u )", width, height, m_width, m_height );

    TextureCreateInfo texInfo( m_imageFormat, m_width, m_height );
    texInfo.sampler = SAMPLER_BILINEAR;

    std::vector<VkImage> images;
    u32 imageCount = 0;
    VK_CHECK( vkGetSwapchainImagesKHR( rg.device, m_handle, &imageCount, nullptr ) );
    images.resize( imageCount );
    VK_CHECK( vkGetSwapchainImagesKHR( rg.device, m_handle, &imageCount, images.data() ) );

    m_textures.resize( images.size() );
    m_submitSemaphores.resize( images.size() );
    for ( size_t i = 0; i < images.size(); ++i )
    {
        VkImageViewCreateInfo viewCI{
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = surfaceFormat.format,
            .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        };
        VkImageView imgView;
        VK_CHECK( vkCreateImageView( rg.device, &viewCI, nullptr, &imgView ) );

        Texture& tex = m_textures[i];
#if USING( DEBUG_BUILD )
        std::string name = "swapchainTex_" + std::to_string( i );
        tex.debugName    = (char*)malloc( name.length() + 1 );
        strcpy( tex.debugName, name.c_str() );
#endif // #if USING( DEBUG_BUILD )
        tex.m_info          = texInfo;
        tex.m_image         = images[i];
        tex.m_imageView     = imgView;
        tex.m_allocation    = nullptr;
        tex.m_sampler       = GetSampler( SAMPLER_BILINEAR );
        tex.m_bindlessIndex = BindlessManager::AddTexture( &tex );
        PG_DEBUG_MARKER_SET_IMAGE_NAME( images[i], "swapchain " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( imgView, "swapchain " + std::to_string( i ) );

        std::string iStr      = std::to_string( i );
        m_submitSemaphores[i] = rg.device.NewSemaphore( "swapchain_submit" + iStr );
    }

    return true;
}

bool Swapchain::Recreate( u32 preferredWidth, u32 preferredHeight )
{
    Free();
    return Create( preferredWidth, preferredHeight );
}

bool Swapchain::AcquireNextImage( const Semaphore& acquireSemaphore )
{
    PGP_ZONE_SCOPEDN( "Swapchain::AcquireNextImage" );
    VkResult res      = vkAcquireNextImageKHR( rg.device, m_handle, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &m_currentImageIdx );
    bool resizeNeeded = res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR;
    PG_ASSERT( res == VK_SUCCESS || resizeNeeded, "vkAcquireNextImageKHR failed with error %d", res );
    return !resizeNeeded;
}

void Swapchain::Free()
{
    rg.device.WaitForIdle();
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    for ( size_t i = 0; i < m_textures.size(); ++i )
    {
        vkDestroyImageView( rg.device, m_textures[i].GetView(), nullptr );
        BindlessManager::RemoveTexture( m_textures[i].m_bindlessIndex );
        m_submitSemaphores[i].Free();
    }
    m_textures.clear();
    m_submitSemaphores.clear();

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
Semaphore Swapchain::GetSubmitSemaphore() const { return m_submitSemaphores[m_currentImageIdx]; }
Texture& Swapchain::GetTexture() { return m_textures[m_currentImageIdx]; }
Texture& Swapchain::GetTextureAt( u32 index ) { return m_textures[index]; }

} // namespace PG::Gfx
