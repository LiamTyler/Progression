#include "renderer/r_init.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_renderpasses.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"
#include <cstring>

VkDebugUtilsMessengerEXT s_debugMessenger;

extern std::vector< const char* > VK_VALIDATION_LAYERS;

namespace PG
{
namespace Gfx
{


static std::vector< std::string > FindMissingValidationLayers( const std::vector< const char* >& layers )
{
    uint32_t layerCount;
    VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &layerCount, nullptr ) );
    std::vector< VkLayerProperties > availableLayers( layerCount );
    VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() ) );

    // LOG( "Available validation layers:\n" );
    // for ( const auto& layerProperties : availableLayers )
    // {
    //     LOG( "\t%s\n", layerProperties.layerName );
    // }

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
        LOG_ERR( "Vulkan message type '%s': '%s'\n", messageTypeString.c_str(), pCallbackData->pMessage );
    }
    else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        LOG_WARN( "Vulkan message type '%s': '%s'\n", messageTypeString.c_str(), pCallbackData->pMessage );
    }
    else
    {
        LOG( "Vulkan message type '%s': '%s'\n", messageTypeString.c_str(), pCallbackData->pMessage );
    }

    return VK_FALSE;
}


static bool CreateDebugUtilsMessengerEXT( const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator )
{
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( r_globals.instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( r_globals.instance, pCreateInfo, pAllocator, &s_debugMessenger ) == VK_SUCCESS;
    }
    else
    {
        return false;
    }
}


static void DestroyDebugUtilsMessengerEXT( const VkAllocationCallbacks* pAllocator = nullptr )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( r_globals.instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( r_globals.instance, s_debugMessenger, pAllocator );
    }
}


static bool CreateInstance()
{
    // struct that holds info about our application. Mainly used by some layers / drivers
    // for labeling debug messages, logging, etc. Possible for drivers to run differently
    // depending on the application that is running.
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr; // pointer to extension information
    appInfo.pApplicationName   = nullptr;
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "Progression";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_2;

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

    std::vector< const char* > validationLayers =
    {
        "VK_LAYER_KHRONOS_validation"
    };
    // Specify global validation layers
#if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = static_cast< uint32_t >( validationLayers.size() );
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else // #if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = 0;
#endif // #else // #if !USING( SHIP_BUILD )

    VkResult ret = vkCreateInstance( &createInfo, nullptr, &r_globals.instance );
    if ( ret == VK_ERROR_EXTENSION_NOT_PRESENT )
    {
        LOG_ERR( "Could not find all of the instance extensions\n" );
    }
    else if ( ret == VK_ERROR_LAYER_NOT_PRESENT )
    {
        auto missingLayers = FindMissingValidationLayers( validationLayers );
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


static bool CreateSynchronizationObjects()
{
    r_globals.presentCompleteSemaphore = r_globals.device.NewSemaphore( "present complete" );
    r_globals.renderCompleteSemaphore  = r_globals.device.NewSemaphore( "render complete" );
    r_globals.computeFence             = r_globals.device.NewFence( true, "compute" );

    return true;
}


static bool CreateDescriptorPool()
{
    VkDescriptorPoolSize poolSizes[11] = {};
    poolSizes[0]  = { VK_DESCRIPTOR_TYPE_SAMPLER, 50 };
    poolSizes[1]  = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 };
    poolSizes[2]  = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 };
    poolSizes[3]  = { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 50 };
    poolSizes[4]  = { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 50 };
    poolSizes[5]  = { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 50 };
    poolSizes[6]  = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 };
    poolSizes[7]  = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50 };
    poolSizes[8]  = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 50 };
    poolSizes[9]  = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 50 };
    poolSizes[10] = { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 50 };

    r_globals.descriptorPool = r_globals.device.NewDescriptorPool( 11, poolSizes, 1000 );

    return r_globals.descriptorPool;
}


static bool CreateSurface()
{
    return glfwCreateWindowSurface( r_globals.instance, GetMainWindow()->GetGLFWHandle(), nullptr, &r_globals.surface ) == VK_SUCCESS;
}


static bool CreateDepthTexture()
{
    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.format  = PixelFormat::DEPTH_32_FLOAT;
    info.width   = r_globals.swapchain.GetWidth();
    info.height  = r_globals.swapchain.GetHeight();
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    r_globals.depthTex = r_globals.device.NewTexture( info, false, "main depth texture" );

    info.format  = PixelFormat::R16_G16_B16_A16_FLOAT;
    info.width   = r_globals.swapchain.GetWidth();
    info.height  = r_globals.swapchain.GetHeight();
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    r_globals.colorTex = r_globals.device.NewTexture( info, false, "main color hdr tex" );

    return r_globals.depthTex && r_globals.colorTex;
}


static bool CreateCommandPoolAndBuffers()
{
    r_globals.commandPools[GFX_CMD_POOL_GRAPHICS]  = r_globals.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, CommandPoolQueueFamily::GRAPHICS, "global graphics" );
    r_globals.commandPools[GFX_CMD_POOL_COMPUTE]   = r_globals.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, CommandPoolQueueFamily::COMPUTE, "global compute" );
    r_globals.commandPools[GFX_CMD_POOL_TRANSIENT] = r_globals.device.NewCommandPool( COMMAND_POOL_TRANSIENT, CommandPoolQueueFamily::GRAPHICS, "global graphics transient" );
    for ( int i = 0; i < GFX_CMD_POOL_TOTAL; ++i )
    {
        if ( !r_globals.commandPools[i] )
        {
            return false;
        }
    }

    r_globals.computeCommandBuffer = r_globals.commandPools[GFX_CMD_POOL_COMPUTE].NewCommandBuffer( "global compute" );
    r_globals.graphicsCommandBuffer = r_globals.commandPools[GFX_CMD_POOL_GRAPHICS].NewCommandBuffer( "global graphics" );
    if ( !r_globals.computeCommandBuffer || !r_globals.graphicsCommandBuffer )
    {
        return false;
    }

    return true;
}


bool R_Init( bool headless, uint32_t width, uint32_t height )
{
    r_globals = {};
    r_globals.headless = headless;
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

    if ( !headless )
    {
        if ( !CreateSurface() )
        {
            LOG_ERR( "Could not create glfw vulkan surface\n" );
            return false;
        }
    }

    if ( !r_globals.physicalDevice.Select( headless ) )
    {
        LOG_ERR( "Could not find any suitable GPU device to use\n" );
        return false;
    }
    {
        LOG( "Using device: '%s'\n", r_globals.physicalDevice.GetName().c_str() );
        auto p = r_globals.physicalDevice.GetProperties();
        LOG( "Using Vulkan Version: %d.%d.%d\n", VK_VERSION_MAJOR( p.apiVersion ), VK_VERSION_MINOR( p.apiVersion ), VK_VERSION_PATCH( p.apiVersion ) );
    }

    r_globals.device = Device::Create( headless );
    if ( !r_globals.device )
    {
        LOG_ERR( "Could not create logical device\n" );
        return false;
    }

    DebugMarker::Init( r_globals.device.GetHandle(), r_globals.physicalDevice.GetHandle() );
    PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( r_globals.physicalDeviceInfo.device, r_globals.physicalDeviceInfo.name );
    PG_DEBUG_MARKER_SET_INSTANCE_NAME( r_globals.instance, "global" );
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( r_globals.device, "default" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( r_globals.device.GraphicsQueue(), "global graphics" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( r_globals.device.ComputeQueue(), "global compute" );
    if ( !headless )
    {
        PG_DEBUG_MARKER_SET_QUEUE_NAME( r_globals.device.PresentQueue(), "presentation" );
    }

    InitSamplers();

    if ( !CreateCommandPoolAndBuffers() )
    {
        LOG_ERR( "Could not create commandPool and commandBuffers\n" );
        return false;
    }

    CreateDescriptorPool();

    if ( !Profile::Init() )
    {
        LOG_ERR( "Could not initialize gpu profiler" );
        return false;
    }

    if ( headless )
    {
        return true;
    }

    if ( !r_globals.swapchain.Create( r_globals.device.GetHandle(), width, height ) )
    {
        LOG_ERR( "Could not create swap chain\n" );
        return false;
    }

    if ( !CreateDepthTexture() )
    {
        LOG_ERR( "Could not create depth texture\n" );
        return false;
    }

    if ( !InitRenderPasses() )
    {
        LOG_ERR( "Could not init renderpass data\n" );
        return false;
    }

    if ( !CreateSynchronizationObjects() )
    {
        LOG_ERR( "Could not create synchronization objects\n" );
        return false;
    }

    return true;
}


void R_Shutdown()
{
    if ( !r_globals.headless )
    {
        r_globals.presentCompleteSemaphore.Free();
        r_globals.renderCompleteSemaphore.Free();
        r_globals.computeFence.Free();
        r_globals.depthTex.Free();
        r_globals.colorTex.Free();
        for ( uint32_t i = 0; i < r_globals.swapchain.GetNumImages(); ++i )
        {
            r_globals.swapchainFramebuffers[i].Free();
        }
        FreeRenderPasses();
        r_globals.swapchain.Free();
        FreeSamplers();
        vkDestroySurfaceKHR( r_globals.instance, r_globals.surface, nullptr );
    }

    Profile::Shutdown();
    for ( int i = 0; i < GFX_CMD_POOL_TOTAL; ++i )
    {
        r_globals.commandPools[i].Free();
    }
    r_globals.descriptorPool.Free();
    r_globals.device.Free();

    DestroyDebugUtilsMessengerEXT();
    vkDestroyInstance( r_globals.instance, nullptr );
}


} // namespace Gfx
} // namespace PG
