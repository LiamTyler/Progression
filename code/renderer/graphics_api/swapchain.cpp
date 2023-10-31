#include "renderer/graphics_api/swapchain.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/vulkan.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include <vector>

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


static SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    SwapChainSupportDetails details;

    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities ) );

    uint32_t formatCount;
    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr ) );

    if ( formatCount != 0 )
    {
        details.formats.resize( formatCount );
        VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() ) );
    }

    uint32_t presentModeCount;
    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr ) );

    if ( presentModeCount != 0 )
    {
        details.presentModes.resize( presentModeCount );
        VK_CHECK_RESULT( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() ) );
    }

    return details;
}


static VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities, uint32_t preferredWidth, uint32_t preferredHeight )
{
    // check if the driver specified the size already
    if ( capabilities.currentExtent.width != UINT32_MAX )
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { preferredWidth, preferredHeight };

        actualExtent.width  = std::max( capabilities.minImageExtent.width, std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
        actualExtent.height = std::max( capabilities.minImageExtent.height, std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

        return actualExtent;
    }
}


static VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes )
{
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;

    //for (const auto& availablePresentMode : availablePresentModes)
    //{
    //    if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
    //    {
    //        mode = availablePresentMode;
    //    }
    //}
    //if ( mode != VK_PRESENT_MODE_MAILBOX_KHR )
    //{
    //    LOG_WARN( "Preferred swap chain format 'VK_PRESENT_MODE_MAILBOX_KHR' not available. Defaulting to VK_PRESENT_MODE_FIFO_KHR" );
    //}

    return mode;
}


static VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats )
{
    PG_ASSERT( availableFormats.size() > 0, "Swap chain has no available formats!" );
    // check if the surface has no preferred format (best case)
    if ( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
    {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for ( const auto& availableFormat : availableFormats )
    {
        if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return availableFormat;
        }
    }

    LOG_WARN( "Format not BGRA8 with SRGB colorspace. Instead format = %d, colorspace = %d", (int)availableFormats[0].format, (int)availableFormats[0].colorSpace );

    return availableFormats[0];
}


namespace PG
{
namespace Gfx
{

bool SwapChain::Create( VkDevice dev, uint32_t preferredWidth, uint32_t preferredHeight )
{
    m_device = dev;
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( rg.physicalDevice.GetHandle(), rg.surface );
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR presentMode = ChooseSwapPresentMode( swapChainSupport.presentModes );

    m_imageFormat = surfaceFormat.format;
    VkExtent2D extent = ChooseSwapExtent( swapChainSupport.capabilities, preferredWidth, preferredHeight );
    m_width  = extent.width;
    m_height = extent.height;
    if ( m_width != preferredWidth || m_height != preferredHeight )
    {
        LOG_WARN( "Could not create swapchain with dimensions %u x %u. Instead got %u x %u", preferredWidth, preferredHeight, m_width, m_height );
    }

    m_numImages = swapChainSupport.capabilities.minImageCount + 1;
    // maxImageCount == 0 means no maximum
    if ( swapChainSupport.capabilities.maxImageCount > 0 )
    {
        m_numImages = std::min( m_numImages, swapChainSupport.capabilities.maxImageCount );
    }
    m_numImages = std::min<uint32_t>( m_numImages, GFX_MAX_SWAPCHAIN_IMAGES );
    if ( m_numImages < swapChainSupport.capabilities.minImageCount )
    {
        LOG_ERR( "Swapchain num images is lower than the minimum for this gpu! Increase GFX_MAX_SWAPCHAIN_IMAGES?" );
    }

    VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface          = rg.surface;
    createInfo.minImageCount    = m_numImages;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1; // always 1 unless doing VR
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // can specify transforms to happen (90 rotation, horizontal flip, etc). None used for now
    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    // only applies if you have to create a new swap chain (like on window resizing)
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if ( vkCreateSwapchainKHR( m_device, &createInfo, nullptr, &m_handle ) != VK_SUCCESS )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( m_handle, "global" );

    // m_numImages is a minimum, the driver is allowed to make more
    VK_CHECK_RESULT( vkGetSwapchainImagesKHR( m_device, m_handle, &m_numImages, nullptr ) );
    PG_ASSERT( m_numImages <= GFX_MAX_SWAPCHAIN_IMAGES, "Increase GFX_MAX_SWAPCHAIN_IMAGES!" );
    VK_CHECK_RESULT( vkGetSwapchainImagesKHR( m_device, m_handle, &m_numImages, m_images.data() ) );

    for ( uint32_t i = 0; i < m_numImages; ++i )
    {
        m_imageViews[i] = CreateImageView( m_images[i], m_imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( m_imageViews[i], "swapchain image " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( m_images[i], "swapchain " + std::to_string( i ) );
    }
    
    return true;
}


uint32_t SwapChain::AcquireNextImage( const Semaphore& presentCompleteSemaphore )
{
    VK_CHECK_RESULT( vkAcquireNextImageKHR( m_device, m_handle, UINT64_MAX, presentCompleteSemaphore.GetHandle(), VK_NULL_HANDLE, &m_currentImage ) );
    return m_currentImage;
}


void SwapChain::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    for ( size_t i = 0; i < m_numImages; ++i )
    {
        vkDestroyImageView( m_device, m_imageViews[i], nullptr );
    }

    vkDestroySwapchainKHR( m_device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}


SwapChain::operator bool() const
{
    return m_handle != VK_NULL_HANDLE;
}


uint32_t SwapChain::GetCurrentImageIndex() const            { return m_currentImage; }
VkFormat SwapChain::GetFormat() const                       { return m_imageFormat; }
uint32_t SwapChain::GetWidth() const                        { return m_width; }
uint32_t SwapChain::GetHeight() const                       { return m_height; }
VkSwapchainKHR SwapChain::GetHandle() const                 { return m_handle; }
uint32_t SwapChain::GetNumImages() const                    { return m_numImages; }
VkImage SwapChain::GetImage( uint32_t index ) const         { return m_images[index]; }
VkImageView SwapChain::GetImageView( uint32_t index ) const { return m_imageViews[index]; }

} // namespace Gfx
} // namespace PG
