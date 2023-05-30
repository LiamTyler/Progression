#include "renderer/r_init.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "core/window.hpp" // must come after pg_to_vulkan_types.hpp for some reason
#include "renderer/r_globals.hpp"
#include "shared/logger.hpp"
#include <cstring>

VkDebugUtilsMessengerEXT s_debugMessenger;
VkDescriptorSetLayout s_emptyDescriptSetLayout;

extern std::vector< const char* > VK_VALIDATION_LAYERS;

namespace PG
{
namespace Gfx
{

VkDescriptorSetLayout GetEmptyDescriptorSetLayout()
{
    return s_emptyDescriptSetLayout;
}


static std::vector< std::string > FindMissingValidationLayers( const std::vector< const char* >& layers )
{
    uint32_t layerCount;
    VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &layerCount, nullptr ) );
    std::vector< VkLayerProperties > availableLayers( layerCount );
    VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() ) );

    // LOG( "Available validation layers:" );
    // for ( const auto& layerProperties : availableLayers )
    // {
    //     LOG( "\t%s", layerProperties.layerName );
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
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( rg.instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( rg.instance, pCreateInfo, pAllocator, &s_debugMessenger ) == VK_SUCCESS;
    }
    else
    {
        return false;
    }
}


static void DestroyDebugUtilsMessengerEXT( const VkAllocationCallbacks* pAllocator = nullptr )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( rg.instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( rg.instance, s_debugMessenger, pAllocator );
    }
}


static bool CreateInstance()
{
    // struct that holds info about our application. Mainly used by some layers / drivers
    // for labeling debug messages, logging, etc. Possible for drivers to run differently
    // depending on the application that is running.
    VkApplicationInfo appInfo  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    appInfo.pNext              = nullptr;
    appInfo.pApplicationName   = nullptr;
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "Progression";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    // non-optional struct that specifies which global extension and validation layers to use
    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
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
    createInfo.enabledExtensionCount   = static_cast<uint32_t>( extensionNames.size() );
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    std::vector< const char* > validationLayers =
    {
#if !USING( SHIP_BUILD )
        "VK_LAYER_KHRONOS_validation"
#endif // #else // #if !USING( SHIP_BUILD )
    };

    createInfo.enabledLayerCount   = static_cast<uint32_t>( validationLayers.size() );
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkResult ret = vkCreateInstance( &createInfo, nullptr, &rg.instance );
    if ( ret == VK_ERROR_EXTENSION_NOT_PRESENT )
    {
        LOG_ERR( "Could not find all of the instance extensions" );
    }
    else if ( ret == VK_ERROR_LAYER_NOT_PRESENT )
    {
        auto missingLayers = FindMissingValidationLayers( validationLayers );
        LOG_ERR( "Could not find the following validation layers:" );
        for ( const auto& layer : missingLayers )
        {
            LOG_ERR( "\t%s", layer.c_str() );
        }
    }
    else if ( ret != VK_SUCCESS )
    {
        LOG_ERR( "Error while creating instance: %d", ret );
    }

    return ret == VK_SUCCESS;
}


static bool SetupDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
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

    rg.descriptorPool = rg.device.NewDescriptorPool( 11, poolSizes, true, 1000 );

    return rg.descriptorPool;
}


static bool CreateSurface()
{
    return glfwCreateWindowSurface( rg.instance, GetMainWindow()->GetGLFWHandle(), nullptr, &rg.surface ) == VK_SUCCESS;
}


static bool CreateCommandPools()
{
    rg.commandPools[GFX_CMD_POOL_GRAPHICS]  = rg.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, CommandPoolQueueFamily::GRAPHICS, "global graphics" );
    rg.commandPools[GFX_CMD_POOL_TRANSIENT] = rg.device.NewCommandPool( COMMAND_POOL_TRANSIENT, CommandPoolQueueFamily::GRAPHICS, "global transient" );
    for ( int i = 0; i < GFX_CMD_POOL_TOTAL; ++i )
    {
        if ( !rg.commandPools[i] ) return false;
    }

    return true;
}


static bool CreatePerFrameData()
{
    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        FrameData& data = rg.frameData[i];
        std::string idxStr = std::to_string( i );
        data.graphicsCmdBuf = rg.commandPools[GFX_CMD_POOL_GRAPHICS].NewCommandBuffer( "graphics" + idxStr );
        data.inFlightFence = rg.device.NewFence( true, "inFlight" + idxStr );
        data.renderCompleteSemaphore = rg.device.NewSemaphore( "renderComplete" + idxStr );
        data.swapchainImgAvailableSemaphore = rg.device.NewSemaphore( "swapImgAvailable" + idxStr );

        if ( !data.graphicsCmdBuf || !data.inFlightFence || !data.renderCompleteSemaphore || !data.swapchainImgAvailableSemaphore )
            return false;
    }

    return true;
}


static void CreateEmptyDescriptorSet()
{
    VkDescriptorSetLayoutCreateInfo emptyLayoutInfo = {};
    emptyLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    emptyLayoutInfo.flags = 0;
    emptyLayoutInfo.pNext = NULL;
    emptyLayoutInfo.bindingCount = 0;
    emptyLayoutInfo.pBindings = NULL;
    vkCreateDescriptorSetLayout( rg.device.GetHandle(), &emptyLayoutInfo, NULL, &s_emptyDescriptSetLayout );
}


bool R_Init( bool headless, uint32_t displayWidth, uint32_t displayHeight )
{
    rg = {};
    rg.headless = headless;
    if ( !CreateInstance() )
    {
        LOG_ERR( "Could not create vulkan instance" );
        return false;
    }

#if !USING( SHIP_BUILD )
    DebugMarker::Init( rg.instance );
    if ( !SetupDebugCallback() )
    {
        LOG_ERR( "Could not setup the debug callback" );
        return false;
    }
#endif // #if !USING( SHIP_BUILD )

    if ( !headless )
    {
        if ( !CreateSurface() )
        {
            LOG_ERR( "Could not create glfw vulkan surface" );
            return false;
        }
    }

    std::vector<PhysicalDevice> pDevices = EnumerateCompatiblePhysicalDevices( rg.instance, rg.surface, headless, false );
    if ( pDevices.empty() )
    {
        LOG_ERR( "No gpus are valid to use, based on the required extensions and features" );
        return false;
    }
    else
    {
        rg.physicalDevice = SelectBestPhysicalDevice( pDevices );
        auto pDevProperties = rg.physicalDevice.GetProperties();
        LOG( "Using device: '%s', Vulkan Version: %u.%u.%u", rg.physicalDevice.GetName().c_str(), pDevProperties.apiVersionMajor, pDevProperties.apiVersionMinor, pDevProperties.apiVersionPatch );
    }

    if ( !rg.device.Create( rg.physicalDevice, headless ) )
    {
        LOG_ERR( "Could not create logical device" );
        return false;
    }
    LoadVulkanExtensions( rg.device.GetHandle() );
    
    PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( rg.physicalDevice, rg.physicalDevice.GetName() );
    PG_DEBUG_MARKER_SET_INSTANCE_NAME( rg.instance, "global" );
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( rg.device, "default" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( rg.device.GetQueue().queue, "queue: GCT" );

    InitSamplers();

    if ( !CreateCommandPools() )
    {
        LOG_ERR( "Could not create commandPools" );
        return false;
    }

    if ( !CreatePerFrameData() )
    {
        LOG_ERR( "CreatePerFrameData failed" );
        return false;
    }

    CreateDescriptorPool();
    CreateEmptyDescriptorSet();

    if ( !Profile::Init() )
    {
        LOG_ERR( "Could not initialize gpu profiler" );
        return false;
    }

    if ( headless )
    {
        return true;
    }

    if ( !rg.swapchain.Create( rg.device.GetHandle(), displayWidth, displayHeight ) )
    {
        LOG_ERR( "Could not create swap chain" );
        return false;
    }

    return true;
}


void R_Shutdown()
{
    if ( !rg.headless )
    {
        for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
        {
            rg.frameData[i].inFlightFence.Free();
            rg.frameData[i].swapchainImgAvailableSemaphore.Free();
            rg.frameData[i].renderCompleteSemaphore.Free();
        }
        
        rg.swapchain.Free();
        FreeSamplers();
        vkDestroySurfaceKHR( rg.instance, rg.surface, nullptr );
    }

    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        rg.frameData[i].graphicsCmdBuf.Free();
    }

    Profile::Shutdown();
    for ( int i = 0; i < GFX_CMD_POOL_TOTAL; ++i )
    {
        rg.commandPools[i].Free();
    }
    vkDestroyDescriptorSetLayout( rg.device.GetHandle(), s_emptyDescriptSetLayout, nullptr );
    rg.descriptorPool.Free();
    rg.device.Free();

    DestroyDebugUtilsMessengerEXT();
    vkDestroyInstance( rg.instance, nullptr );
}


} // namespace Gfx
} // namespace PG
