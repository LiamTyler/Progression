#include "renderer/vulkan.hpp"
#include "core/platform_defines.hpp"
#include <vulkan/vulkan.h>
#include "core/window.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <vector>


std::vector< const char* > VK_VALIDATION_LAYERS =
{
    "VK_LAYER_KHRONOS_validation"
};

std::vector< const char* > VK_DEVICE_EXTENSIONS =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace PG
{
namespace Gfx
{

RenderState g_renderState;

bool PhysicalDeviceInfo::ExtensionSupported( const std::string& extensionName ) const
{
    return std::find( availableExtensions.begin(), availableExtensions.end(), extensionName ) != availableExtensions.end();
}

static std::vector< std::string > FindMissingValidationLayers( const std::vector< const char* >& layers )
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    std::vector< VkLayerProperties > availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    LOG( "Available validation layers:\n" );
    for ( const auto& layerProperties : availableLayers )
    {
        LOG( "\t%s\n", layerProperties.layerName );
    }

    std::vector< std::string > missing;
    for ( const auto& layer : layers )
    {
        bool found = false;
        for ( const auto& availableLayer : availableLayers )
        {
            if ( strcmp( layer, availableLayer.layerName ) == 0 )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            missing.push_back( layer );
        }
    }

    return missing;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    PG_UNUSED( pUserData );

    std::string messageTypeString;
    switch ( messageType )
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            return VK_FALSE;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            messageTypeString = "Validation";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            messageTypeString = "Performance";
            break;
        default:
            messageTypeString = "Unknown";
    }

    if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
    {
        LOG_ERR( "Vulkan message type '%s': '%s'", messageTypeString.c_str(), pCallbackData->pMessage );
    }
    else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        LOG_WARN( "Vulkan message type '%s': '%s'", messageTypeString.c_str(), pCallbackData->pMessage );
    }
    else
    {
        LOG( "Vulkan message type '%s': '%s'", messageTypeString.c_str(), pCallbackData->pMessage );
    }

    return VK_FALSE;
}

/** Since the createDebugUtilsMessenger function is from an extension, it is not loaded
 * automatically. Look up its address manually and call it.
 */
static bool CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator )
{
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( g_renderState.instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( g_renderState.instance, pCreateInfo, pAllocator, &g_renderState.debugMessenger ) == VK_SUCCESS;
    }
    else
    {
        return false;
    }
}

/** Same thing as the CreateDebugUtilsMessenger; load the function manually and call it
 */
static void DestroyDebugUtilsMessengerEXT( const VkAllocationCallbacks* pAllocator = nullptr )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( g_renderState.instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( g_renderState.instance, g_renderState.debugMessenger, pAllocator );
    }
}

/** \brief Find and select the first avaiable queues for graphics and presentation
* Queues are where commands get submitted to and are processed asynchronously. Some queues
* might only be usable for certain operations, like graphics or memory operations.
* Currently we just need 1 queue for graphics commands, and 1 queue for
* presenting the images we create to the surface.
*/
static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ) && !indices.IsComplete(); ++i )
    {
        // check if the queue supports graphics operations
        if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            // check if the queue supports presenting images to the surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if ( queueFamilies[i].queueCount > 0 && presentSupport )
            {
                indices.presentFamily = i;
                indices.graphicsFamily = i;
            }
        }

        // check for dedicated compute queue
        if ( queueFamilies[i].queueCount > 0 && ( queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) && ( ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 ) )
        {
            indices.computeFamily = i;
        }
    }

    // if there existed no queuefamily for both graphics and presenting, then look for separate ones
    if ( indices.graphicsFamily == ~0u )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ) && !indices.IsComplete(); ++i )
        {
            // check if the queue supports graphics operations
            if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.graphicsFamily = i;
            }

            // check if the queue supports presenting images to the surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if ( queueFamilies[i].queueCount > 0 && presentSupport )
            {
                indices.presentFamily = i;
            }
        }
    }

    // Look for first compute queue if no dedicated one exists
    if ( !indices.IsComplete() )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
        {
            // check if the queue supports graphics operations
            if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                indices.computeFamily = i;
                break;
            }
        }
    }

    return indices;
}

static bool CheckPhysicalDeviceExtensionSupport( const PhysicalDeviceInfo& info )
{
    for ( const auto& extension : VK_DEVICE_EXTENSIONS )
    {
        if ( !info.ExtensionSupported( extension ) )
        {
            return false;
        }
    }

    return true;
}

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector< VkSurfaceFormatKHR > formats;
    std::vector< VkPresentModeKHR > presentModes;
};

static VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities )
{
    // check if the driver specified the size already
    if ( capabilities.currentExtent.width != std::numeric_limits< uint32_t >::max() )
    {
        return capabilities.currentExtent;
    }
    else
    {
        // select the closest feasible resolution possible to the window size
        int SW = GetMainWindow()->Width();
        int SH = GetMainWindow()->Height();
        glfwGetFramebufferSize( GetMainWindow()->GetGLFWHandle(), &SW, &SH );

        VkExtent2D actualExtent =
        {
            static_cast< uint32_t >( SW ),
            static_cast< uint32_t >( SH )
        };

        actualExtent.width = std::max( capabilities.minImageExtent.width,
                                       std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
        actualExtent.height = std::max( capabilities.minImageExtent.height,
                                        std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

        return actualExtent;
    }
}

static VkPresentModeKHR ChooseSwapPresentMode( const std::vector< VkPresentModeKHR >& availablePresentModes )
{
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            mode = availablePresentMode;
        }
    }
    if ( mode != VK_PRESENT_MODE_MAILBOX_KHR )
    {
        LOG_WARN( "Preferred swap chain format 'VK_PRESENT_MODE_MAILBOX_KHR' not available. Defaulting to VK_PRESENT_MODE_FIFO_KHR\n" );
    }

    return mode;
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector< VkSurfaceFormatKHR >& availableFormats )
{
    // check if the surface has no preferred format (best case)
    // if ( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
    // {
    //     return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    // }

    for ( const auto& availableFormat : availableFormats )
    {
        if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return availableFormat;
        }
    }

    LOG_WARN( "Format not RGBA8 with SRGB colorspace. Instead format = %d, colorspace = %d\n", availableFormats[0].format, availableFormats[0].colorSpace );

    return availableFormats[0];
}

static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 )
    {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 )
    {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
    }

    return details;
}

/** Return a rating of how good this device is. 0 is incompatible, and the higher the better. */
static int RatePhysicalDevice( const PhysicalDeviceInfo& deviceInfo )
{
    // check the required features first: queueFamilies, extension support,
    // and swap chain support
    bool extensionsSupported = CheckPhysicalDeviceExtensionSupport( deviceInfo );

    bool swapChainAdequate = false;
    if ( extensionsSupported )
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( deviceInfo.device, g_renderState.surface );
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    if ( !deviceInfo.indices.IsComplete() || !extensionsSupported || !swapChainAdequate || !deviceInfo.deviceFeatures.samplerAnisotropy )
    {
        return 0;
    }

    int score = 10;
    if ( deviceInfo.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    {
        score += 1000;
    }
    
    return score;
}

static bool CreateInstance()
{
    // struct that holds info about our application. Mainly used by some layers / drivers
    // for labeling debug messages, logging, etc. Possible for drivers to run differently
    // depending on the application that is running.
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr; // pointer to extension information
    appInfo.pApplicationName   = "PG";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "PG";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    // non-optional struct that specifies which global extension and validation layers to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;

    // Vulkan by itself doesn't know how to do any platform specifc things, so we do need
    // extensions. Specifically, we at least need the ones to interface with the windowing API,
    // so ask glfw for the extensions needed for this. These are global to the program.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    std::vector<const char*> extensionNames( glfwExtensions, glfwExtensions + glfwExtensionCount );

#if !USING( SHIP_BUILD )
    // Also want the debug utils extension so we can print out layer messages
    extensionNames.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif // #if !USING( SHIP_BUILD )

    createInfo.enabledExtensionCount   = static_cast< uint32_t >( extensionNames.size() );
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    // Specify global validation layers
#if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = static_cast< uint32_t >( VK_VALIDATION_LAYERS.size() );
    createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
#else // #if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = 0;
#endif // #else // #if !USING( SHIP_BUILD )

    VkResult ret = vkCreateInstance( &createInfo, nullptr, &g_renderState.instance );
    if ( ret == VK_ERROR_EXTENSION_NOT_PRESENT )
    {
        LOG_ERR( "Could not find all of the instance extensions\n" );
    }
    else if ( ret == VK_ERROR_LAYER_NOT_PRESENT )
    {
        auto missingLayers = FindMissingValidationLayers( VK_VALIDATION_LAYERS );
        LOG_ERR( "Could not find the following validation layers:\n" );
        for ( const auto& layer : missingLayers )
        {
            LOG_ERR( "\t%s\n", layer.c_str() );
        }
    }
    else if ( ret != VK_SUCCESS )
    {
        LOG_ERR( "Error while creating instance: %d\n", ret );
    }

    return ret == VK_SUCCESS;
}

static bool SetupDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; // general verbose debug info
    
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    return CreateDebugUtilsMessengerEXT( &createInfo, nullptr );
}

static bool CreateSurface()
{
    return glfwCreateWindowSurface( g_renderState.instance, GetMainWindow()->GetGLFWHandle(), nullptr, &g_renderState.surface ) == VK_SUCCESS;
}

static bool PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( g_renderState.instance, &deviceCount, nullptr );

    if ( deviceCount == 0 )
    {
        return false;
    }

    std::vector< VkPhysicalDevice > devices( deviceCount );
    vkEnumeratePhysicalDevices( g_renderState.instance, &deviceCount, devices.data() );

    std::vector< PhysicalDeviceInfo > deviceInfos( deviceCount );
    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        deviceInfos[i].device  = devices[i];
        vkGetPhysicalDeviceProperties( devices[i], &deviceInfos[i].deviceProperties );
        vkGetPhysicalDeviceFeatures( devices[i], &deviceInfos[i].deviceFeatures );
        deviceInfos[i].deviceFeatures.samplerAnisotropy = VK_TRUE;

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( deviceInfos[i].device, nullptr, &extensionCount, nullptr );
        std::vector< VkExtensionProperties > availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( deviceInfos[i].device, nullptr, &extensionCount, availableExtensions.data() );
        deviceInfos[i].availableExtensions.resize( availableExtensions.size() );
        for ( size_t ext = 0; ext < availableExtensions.size(); ++ext )
        {
            deviceInfos[i].availableExtensions[ext] = availableExtensions[ext].extensionName;
        }

        deviceInfos[i].name    = deviceInfos[i].deviceProperties.deviceName;
        deviceInfos[i].indices = FindQueueFamilies( devices[i], g_renderState.surface );
        deviceInfos[i].score   = RatePhysicalDevice( deviceInfos[i] );
    }

    // sort and select the best GPU available
    std::sort( deviceInfos.begin(), deviceInfos.end(), []( const auto& lhs, const auto& rhs ) { return lhs.score > rhs.score; } );

    auto& physicalInfo = g_renderState.physicalDeviceInfo;
    physicalInfo       = deviceInfos[0];

    if ( physicalInfo.score <= 0 )
    {
        physicalInfo.device = VK_NULL_HANDLE;
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties( physicalInfo.device, &physicalInfo.memProperties );
    
    return true;
}

static bool CreateRenderPass()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorAttachmentDescriptors[0].format      = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
    renderPassDesc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::PRESENT_SRC_KHR;
    renderPassDesc.depthAttachmentDescriptor.format          = PixelFormat::DEPTH_32_FLOAT;
    renderPassDesc.depthAttachmentDescriptor.loadAction      = LoadAction::CLEAR;
    renderPassDesc.depthAttachmentDescriptor.storeAction     = StoreAction::DONT_CARE;
    renderPassDesc.depthAttachmentDescriptor.finalLayout     = ImageLayout::PRESENT_SRC_KHR;

    g_renderState.renderPass = g_renderState.device.NewRenderPass( renderPassDesc, "final output" );
    return g_renderState.renderPass;
}

static bool CreateDepthTexture()
{
    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.format  = PixelFormat::DEPTH_32_FLOAT;
    info.width   = g_renderState.swapChain.extent.width;
    info.height  = g_renderState.swapChain.extent.height;
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    g_renderState.depthTex = g_renderState.device.NewTexture( info, false, "main depth texture" );
    return g_renderState.depthTex;
}

static bool CreateSwapChainFrameBuffers()
{
    VkImageView attachments[2];
    attachments[1] = g_renderState.depthTex.GetView();

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = g_renderState.renderPass.GetHandle();
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = g_renderState.swapChain.extent.width;
    framebufferInfo.height          = g_renderState.swapChain.extent.height;
    framebufferInfo.layers          = 1;

    g_renderState.swapChainFramebuffers.resize( g_renderState.swapChain.images.size() );
    for ( size_t i = 0; i < g_renderState.swapChain.images.size(); ++i )
    {
        attachments[0] = g_renderState.swapChain.imageViews[i];
        g_renderState.swapChainFramebuffers[i] = g_renderState.device.NewFramebuffer( framebufferInfo, "swapchain " + std::to_string( i ) );
        if ( !g_renderState.swapChainFramebuffers[i] )
        {
            return false;
        }
    }

    return true;
}

static bool CreateCommandPoolAndBuffers()
{
    g_renderState.graphicsCommandPool  = g_renderState.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, CommandPoolQueueFamily::GRAPHICS, "global graphics" );
    g_renderState.computeCommandPool   = g_renderState.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, CommandPoolQueueFamily::COMPUTE, "global compute" );
    g_renderState.transientCommandPool = g_renderState.device.NewCommandPool( COMMAND_POOL_TRANSIENT, CommandPoolQueueFamily::GRAPHICS, "global graphics transient" );
    if ( !g_renderState.graphicsCommandPool || !g_renderState.transientCommandPool || !g_renderState.computeCommandPool )
    {
        return false;
    }
    g_renderState.computeCommandBuffer = g_renderState.computeCommandPool.NewCommandBuffer( "compute" );

    g_renderState.graphicsCommandBuffer = g_renderState.graphicsCommandPool.NewCommandBuffer( "global graphics" );
    if ( !g_renderState.graphicsCommandBuffer )
    {
        return false;
    }

    return true;
}

static bool CreateSynchronizationObjects()
{
    g_renderState.presentCompleteSemaphore = g_renderState.device.NewSemaphore( "present complete" );
    g_renderState.renderCompleteSemaphore  = g_renderState.device.NewSemaphore( "render complete" );
    g_renderState.computeFence             = g_renderState.device.NewFence( true, "compute" );

    return true;
}

bool SwapChain::Create( VkDevice dev )
{
    device = dev;
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( g_renderState.physicalDeviceInfo.device, g_renderState.surface );

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR   presentMode   = ChooseSwapPresentMode( swapChainSupport.presentModes );

    imageFormat   = surfaceFormat.format;
    extent        = ChooseSwapExtent( swapChainSupport.capabilities );

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                  = g_renderState.surface;
    createInfo.minImageCount            = imageCount;
    createInfo.imageFormat              = surfaceFormat.format;
    createInfo.imageColorSpace          = surfaceFormat.colorSpace;
    createInfo.imageExtent              = extent;
    createInfo.imageArrayLayers         = 1; // always 1 unless doing VR
    createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& indices = g_renderState.physicalDeviceInfo.indices;
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily, indices.computeFamily };

    if ( indices.graphicsFamily != indices.presentFamily )
    {
        LOG_WARN( "Graphics queue is not the same as the presentation queue! Possible performance drop\n" );
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 3;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // can specify transforms to happen (90 rotation, horizontal flip, etc). None used for now
    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    // only applies if you have to create a new swap chain (like on window resizing)
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if ( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapChain, "global" );

    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
    images.resize( imageCount );
    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, images.data() );

    // image views
    imageViews.resize( images.size() );

    for ( size_t i = 0; i < images.size(); ++i )
    {
        imageViews[i] = CreateImageView( images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1 );
        PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( imageViews[i], "swapchain image " + std::to_string( i ) );
        PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( images[i], "swapchain " + std::to_string( i ) );
    }
    
    return true;
}

uint32_t SwapChain::AcquireNextImage( const Semaphore& presentCompleteSemaphore )
{
    vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, presentCompleteSemaphore.GetHandle(), VK_NULL_HANDLE, &currentImage );

    return currentImage;
}

bool VulkanInit()
{
    if ( !CreateInstance() )
    {
        LOG_ERR( "Could not create vulkan instance\n" );
        return false;
    }

#if !USING( SHIP_BUILD )
    if ( !SetupDebugCallback() )
    {
        LOG_ERR( "Could not setup the debug callback\n" );
        return false;
    }
#endif // #if !USING( SHIP_BUILD )

    if ( !CreateSurface() )
    {
        LOG_ERR( "Could not create glfw vulkan surface\n" );
        return false;
    }

    if ( !PickPhysicalDevice() )
    {
        LOG_ERR( "Could not find any suitable GPU device to use\n" );
        return false;
    }

    g_renderState.device = Device::CreateDefault();
    if ( !g_renderState.device )
    {
        LOG_ERR( "Could not create logical device\n" );
        return false;
    }
    r_globals.device = &g_renderState.device;

    DebugMarker::Init( g_renderState.device.GetHandle(), g_renderState.physicalDeviceInfo.device );
    LOG( "Using device: '%s'\n", g_renderState.physicalDeviceInfo.name.c_str() );

    PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( g_renderState.physicalDeviceInfo.device, g_renderState.physicalDeviceInfo.name );
    PG_DEBUG_MARKER_SET_INSTANCE_NAME( g_renderState.instance, "global" );
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( g_renderState.device, "default" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( g_renderState.device.GraphicsQueue(), "graphics" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( g_renderState.device.PresentQueue(), "presentation" );

    if ( !g_renderState.swapChain.Create( g_renderState.device.GetHandle() ) )
    {
        LOG_ERR( "Could not create swap chain\n" );
        return false;
    }

    if ( !CreateRenderPass() )
    {
        LOG_ERR( "Could not create render pass\n" );
        return false;
    }

    if ( !CreateDepthTexture() )
    {
        LOG_ERR( "Could not create depth texture\n" );
        return false;
    }

    if ( !CreateSwapChainFrameBuffers() )
    {
        LOG_ERR( "Could not create swap chain framebuffers\n" );
        return false;
    }

    if ( !CreateCommandPoolAndBuffers() )
    {
        LOG_ERR( "Could not create commandPool / buffers\n" );
        return false;
    }

    if ( !CreateSynchronizationObjects() )
    {
        LOG_ERR( "Could not create synchronization objects\n" );
        return false;
    }

    return true;
}

void VulkanShutdown()
{
    VkDevice dev = g_renderState.device.GetHandle();

    g_renderState.presentCompleteSemaphore.Free();
    g_renderState.renderCompleteSemaphore.Free();
    g_renderState.computeFence.Free();

    g_renderState.depthTex.Free();

    g_renderState.graphicsCommandPool.Free();
    g_renderState.computeCommandPool.Free();
    g_renderState.transientCommandPool.Free();
    for ( auto framebuffer : g_renderState.swapChainFramebuffers )
    {
        framebuffer.Free();
    }

    g_renderState.renderPass.Free();

    for ( size_t i = 0; i < g_renderState.swapChain.imageViews.size(); ++i )
    {
        vkDestroyImageView( dev, g_renderState.swapChain.imageViews[i], nullptr );
    }

    vkDestroySwapchainKHR( dev, g_renderState.swapChain.swapChain, nullptr);
    g_renderState.device.Free();
    DestroyDebugUtilsMessengerEXT();
    vkDestroySurfaceKHR( g_renderState.instance, g_renderState.surface, nullptr );
    vkDestroyInstance( g_renderState.instance, nullptr );
}

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    auto& memProperties = g_renderState.physicalDeviceInfo.memProperties;
    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
    {
        if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
        {
            return i;
        }
    }

    PG_ASSERT( false );

    return ~0u;
}

void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layers )
{
    CommandBuffer cmdBuf = g_renderState.transientCommandPool.NewCommandBuffer( "One time TransitionImageLayout" );
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = layers;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = 0;
    PG_UNUSED( format );

    VkPipelineStageFlags srcStage, dstStage;
    if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        PG_ASSERT( false, "The transition barriers are unknown for the given old and new layouts" );
    }

    cmdBuf.PipelineBarrier( srcStage, dstStage, barrier );

    cmdBuf.EndRecording();
    g_renderState.device.Submit( cmdBuf );
    g_renderState.device.WaitForIdle();
    cmdBuf.Free();
}

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( g_renderState.physicalDeviceInfo.device, format, &props );
    return ( props.optimalTilingFeatures & requestedSupport ) == requestedSupport;
}

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layers )
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image        = image;
    PG_ASSERT( layers == 1 || layers == 6 );
    viewCreateInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
    if ( layers == 6 )
    {
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    viewCreateInfo.format       = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // specify image purpose and which part to access
    viewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel   = 0;
    viewCreateInfo.subresourceRange.levelCount     = mipLevels;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount     = layers;

    VkImageView view;
    VkResult res = vkCreateImageView( g_renderState.device.GetHandle(), &viewCreateInfo, nullptr, &view );
    PG_ASSERT( res == VK_SUCCESS );

    return view;
}

} // namespace Gfx
} // namespace PG
