#include "renderer/r_init.hpp"
#include "core/dvars.hpp"
#include "core/window.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/features.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#include "shared/logger.hpp"
#include <cstring>
#include <iostream>

#include "SDL3/SDL_vulkan.h"
#if USING( DEVELOPMENT_BUILD )
#include "ui/ui_text.hpp"
#endif // #if USING( DEVELOPMENT_BUILD )

static VkDebugUtilsMessengerEXT s_debugMessenger = VK_NULL_HANDLE;

namespace PG::Gfx
{

Dvar r_shaderDebugPrint( "r_shaderDebugPrint", true, "Enable/disable logging of shader debug printf messages" );

#if !USING( SHIP_BUILD )
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
    PG_UNUSED( pUserData );

#if USING( SHADER_DEBUG_PRINTF )
    if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT && pCallbackData->pMessageIdName )
    {
        if ( strstr( pCallbackData->pMessageIdName, "DEBUG-PRINTF" ) )
        {
            if ( !r_shaderDebugPrint.GetBool() )
                return VK_FALSE;

            std::string_view msg = pCallbackData->pMessage;
            size_t idx           = msg.find( '\n' ); // seems to change with different vk versions?
            if ( idx != std::string_view::npos )
            {
                idx += 1;
                msg = msg.substr( idx );
            }

            if ( msg.substr( 0, 3 ) == "#2D" )
            {
                msg = msg.substr( 3 );
                if ( msg.empty() )
                    return VK_FALSE;

                UI::Text::TextDrawInfo tInfo;
                tInfo.pos   = vec2( 0.9, 0 );
                tInfo.color = vec4( 0, 1, 0, 1 );
                if ( msg[0] == '(' )
                {
                    size_t commaIdx    = msg.find( ',' );
                    size_t endParenIdx = msg.find( ')' );
                    if ( commaIdx != std::string_view::npos && endParenIdx != std::string_view::npos && commaIdx < endParenIdx )
                    {
                        tInfo.pos.x = (float)atof( msg.data() + 1 );
                        tInfo.pos.y = (float)atof( msg.data() + commaIdx + 1 );
                        msg         = msg.substr( endParenIdx + 1 );
                    }
                }

                UI::Text::Draw2D( tInfo, msg.data() );
            }
            else
            {
                Logger_Log( LogSeverity::DEBUG, TerminalColorCode::YELLOW, TerminalEmphasisCode::NONE, "SHADER PRINTF: %s", msg.data() );
            }
            return VK_FALSE;
        }
    }
#endif // #if USING( SHADER_DEBUG_PRINTF )

    if ( messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT && messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT )
        return VK_FALSE;

    const char* messageTypeString;
    switch ( messageType )
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: messageTypeString = "General"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: messageTypeString = "Validation"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: messageTypeString = "Performance"; break;
    default: messageTypeString = "Unknown";
    }

    if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
    {
        LOG_ERR(
            "%s: %d - %s: %s", messageTypeString, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage );
    }
    else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        // skip annoying messages WARNING-Setting-Limit-Adjusted and about debug printf
        if ( pCallbackData->messageIdNumber == -2030147807 || pCallbackData->messageIdNumber == 2132353751 )
            return VK_FALSE;
        LOG_WARN(
            "%s: %d - %s: %s", messageTypeString, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage );
    }
    else
    {
        LOG( "%s: %d - %s: %s", messageTypeString, pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage );
    }

    return VK_FALSE;
}
#endif // #if !USING( SHIP_BUILD )

static void InitCommandObjects()
{
    CommandPoolCreateInfo cmdPoolCI;
    cmdPoolCI.flags     = COMMAND_POOL_RESET_COMMAND_BUFFER;
    cmdPoolCI.queueType = QueueType::GRAPHICS;

    if ( !rg.headless )
    {
        for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
        {
            rg.frameData[i].cmdPool          = rg.device.NewCommandPool( cmdPoolCI, "frame" + std::to_string( i ) );
            rg.frameData[i].primaryCmdBuffer = rg.frameData[i].cmdPool.NewCommandBuffer( "primary" + std::to_string( i ) );
        }
    }

    cmdPoolCI.flags |= COMMAND_POOL_TRANSIENT;
    rg.immediateCmdPool   = rg.device.NewCommandPool( cmdPoolCI, "immediate" );
    rg.immediateCmdBuffer = rg.immediateCmdPool.NewCommandBuffer( "immediate" );
}

static void InitSyncObjects()
{
    if ( !rg.headless )
    {
        for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
        {
            std::string iStr                       = std::to_string( i );
            rg.frameData[i].acquireImageSemaphore  = rg.device.NewSemaphore( "acquireImageSemaphore_" + iStr );
            rg.frameData[i].renderingCompleteFence = rg.device.NewFence( true, "renderingCompleteFence_" + iStr );
        }
    }

    rg.immediateFence = rg.device.NewFence( false, "immediate" );
}

static bool CreateInstance()
{
    PGP_ZONE_SCOPEDN( "CreateInstance" );

    VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName             = "Progression",
        .applicationVersion           = 1,
        .apiVersion                   = VK_API_VERSION_1_3 };

    std::vector<const char*> instanceExtensionsToEnable;

    if ( !rg.headless )
    {
        u32 sdlInstanceExtensionsCount{ 0 };
        const char* const* sdlInstanceExtensions{ SDL_Vulkan_GetInstanceExtensions( &sdlInstanceExtensionsCount ) };
        PG_ASSERT( sdlInstanceExtensions != nullptr, "SDL_Vulkan_GetInstanceExtensions failed '%s'", SDL_GetError() );
        for ( u32 i = 0; i < sdlInstanceExtensionsCount; ++i )
            instanceExtensionsToEnable.push_back( sdlInstanceExtensions[i] );
    }

    VkInstanceCreateInfo instanceCI{
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
    };
    void* chain = nullptr;

#if !USING( SHIP_BUILD )
    u32 availableInstanceExtensionCount;
    VK_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &availableInstanceExtensionCount, nullptr ) );

    std::vector<VkExtensionProperties> availableInstanceExtensions( availableInstanceExtensionCount );
    VK_CHECK( vkEnumerateInstanceExtensionProperties( nullptr, &availableInstanceExtensionCount, availableInstanceExtensions.data() ) );

    enum InstanceExtensions
    {
        DEBUG_UTILS,
        INSTANCE_EXTENSIONS_COUNT
    };

    struct Item
    {
        const char* name;
        bool toEnable; // to give a single source of enabling/disabling items, without commenting out several things

        constexpr Item( const char* n, bool b ) : name( n ), toEnable( b ) {}
    };

    static constexpr std::array extensionsToQuery = std::array{
        Item{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, USING( DEVELOPMENT_BUILD ) },
        // Item{ VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true }, //  functionality also used, but core since vulkan 1.1
    };
    bool extensionAvailability[INSTANCE_EXTENSIONS_COUNT] = {};

    for ( const VkExtensionProperties& availableExtension : availableInstanceExtensions )
    {
        for ( u32 i = 0; i < INSTANCE_EXTENSIONS_COUNT; ++i )
        {
            if ( !strcmp( availableExtension.extensionName, extensionsToQuery[i].name ) )
            {
                extensionAvailability[i] = true;
                break;
            }
        }
    }

#if USING( DEVELOPMENT_BUILD )
    rg.debugUtilsEnabled = false;
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsCI;
    if ( extensionsToQuery[DEBUG_UTILS].toEnable )
    {
        if ( extensionAvailability[DEBUG_UTILS] )
        {
            rg.debugUtilsEnabled = true;
            instanceExtensionsToEnable.push_back( extensionsToQuery[DEBUG_UTILS].name );

            debugUtilsCI = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

            debugUtilsCI.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugUtilsCI.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            debugUtilsCI.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            // debugUtilsCI.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

            debugUtilsCI.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
            debugUtilsCI.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugUtilsCI.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugUtilsCI.pfnUserCallback = DebugCallback;
            debugUtilsCI.pNext           = chain;
            chain                        = &debugUtilsCI;
        }
        else
        {
            LOG_WARN( "This computer does not support %s. This means the debug messenger callback and debug markers will be disabled",
                extensionsToQuery[DEBUG_UTILS].name );
        }
    }
#endif // USING( DEVELOPMENT_BUILD )

    u32 supportedLayerCount;
    vkEnumerateInstanceLayerProperties( &supportedLayerCount, nullptr );

    std::vector<VkLayerProperties> supportedLayers( supportedLayerCount );
    vkEnumerateInstanceLayerProperties( &supportedLayerCount, supportedLayers.data() );

    enum InstanceLayers
    {
        VALIDATION,
        INSTANCE_LAYERS_COUNT
    };

    static constexpr std::array layersToQuery = std::array{
        Item{ "VK_LAYER_KHRONOS_validation", USING( DEVELOPMENT_BUILD ) },
    };
    bool layerAvailability[INSTANCE_LAYERS_COUNT] = {};

    for ( const VkLayerProperties& supportedLayer : supportedLayers )
    {
        for ( u32 i = 0; i < INSTANCE_LAYERS_COUNT; ++i )
        {
            if ( !strcmp( supportedLayer.layerName, layersToQuery[i].name ) )
            {
                layerAvailability[i] = true;
                break;
            }
        }
    }

    std::vector<const char*> layersToEnable;
    std::vector<VkValidationFeatureEnableEXT> validationFeatureEnables;
    if ( layersToQuery[VALIDATION].toEnable )
    {
        if ( layerAvailability[VALIDATION] )
        {
            layersToEnable.push_back( layersToQuery[VALIDATION].name );

#if USING( SHADER_DEBUG_PRINTF )
            validationFeatureEnables.push_back( VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT );
#endif // #if USING( SHADER_DEBUG_PRINTF )
        }
        else
            LOG_WARN( "Validation layer not available? Continuing without it enabled" );
    }

    VkValidationFeaturesEXT validationFeatures{ VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
    validationFeatures.enabledValidationFeatureCount = (u32)validationFeatureEnables.size();
    validationFeatures.pEnabledValidationFeatures    = validationFeatureEnables.data();
    validationFeatures.pNext                         = chain;
    chain                                            = &validationFeatures;

    instanceCI.enabledLayerCount   = (u32)layersToEnable.size();
    instanceCI.ppEnabledLayerNames = layersToEnable.data();
#endif // #if !USING( SHIP_BUILD )

    instanceCI.enabledExtensionCount   = (u32)instanceExtensionsToEnable.size();
    instanceCI.ppEnabledExtensionNames = instanceExtensionsToEnable.data();
    instanceCI.pNext                   = chain;
    VK_CHECK( vkCreateInstance( &instanceCI, nullptr, &rg.instance ) );

#if USING( DEVELOPMENT_BUILD )
    s_debugMessenger = VK_NULL_HANDLE;

    if ( rg.debugUtilsEnabled )
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( rg.instance, "vkCreateDebugUtilsMessengerEXT" );
        if ( !func )
        {
            LOG_ERR( "Failed to get vkCreateDebugUtilsMessengerEXT somehow" );
            return false;
        }
        VK_CHECK( func( rg.instance, &debugUtilsCI, nullptr, &s_debugMessenger ) );
    }
#endif // #if USING( DEVELOPMENT_BUILD )

    return true;
}

bool SelectPhysicalDevice()
{
    PGP_ZONE_SCOPEDN( "SelectPhysicalDevice" );
    PhysicalDeviceExtensions::UpdateClassifications( rg.headless );

    u32 deviceCount = 0;
    VK_CHECK( vkEnumeratePhysicalDevices( rg.instance, &deviceCount, nullptr ) );

    if ( deviceCount == 0 )
    {
        LOG_ERR( "Failed to enumerate any physical devices!" );
        return false;
    }

    std::vector<VkPhysicalDevice> pVKDevices( deviceCount );
    VK_CHECK( vkEnumeratePhysicalDevices( rg.instance, &deviceCount, pVKDevices.data() ) );

    std::vector<PhysicalDevice> pDevices( deviceCount );
    std::vector<std::pair<u32, int>> suitableDevicesAndScores;
    suitableDevicesAndScores.reserve( deviceCount );

    for ( u32 deviceIdx = 0; deviceIdx < deviceCount; ++deviceIdx )
    {
        if ( !pDevices[deviceIdx].Init( pVKDevices[deviceIdx] ) )
            continue;

        suitableDevicesAndScores.emplace_back( deviceIdx, pDevices[deviceIdx].GetMetadata()->suitabilityScore );
    }

    if ( suitableDevicesAndScores.empty() )
    {
        LOG_ERR( "No suitable devices found! Logging reason for insuitability for each device." );
        for ( u32 deviceIdx = 0; deviceIdx < deviceCount; ++deviceIdx )
        {
            pDevices[deviceIdx].LogReasonsForInsuitability();
        }
        return false;
    }

    int bestScore = suitableDevicesAndScores[0].second;
    u32 bestIndex = suitableDevicesAndScores[0].first;
    for ( u32 i = 1; i < suitableDevicesAndScores.size(); ++i )
    {
        u32 devIdx = suitableDevicesAndScores[i].first;
        int score  = suitableDevicesAndScores[i].second;
        if ( score > bestScore )
        {
            bestIndex = devIdx;
            bestScore = score;
        }
    }

    rg.physicalDevice = pDevices[bestIndex];

    return true;
}

bool R_Init( bool headless, u32 displayWidth, u32 displayHeight )
{
    PGP_ZONE_SCOPEDN( "R_Init" );
    rg.currentFrameIdx = rg.totalFrameCount = 0;
    rg.headless                             = headless;

    if ( !CreateInstance() )
    {
        LOG_ERR( "Failed to create vulkan instance" );
        return false;
    }

    rg.surface = VK_NULL_HANDLE;
    if ( !headless )
    {
        PGP_ZONE_SCOPEDN( "CreateSurfaceFromWindow" );
        if ( !SDL_Vulkan_CreateSurface( GetMainWindow()->GetHandle(), rg.instance, nullptr, &rg.surface ) )
        {
            LOG_ERR( "SDL_Vulkan_CreateSurface failed. Error: '%s'", SDL_GetError() );
        }
    }

    if ( !SelectPhysicalDevice() )
    {
        LOG_ERR( "No compatible physical device found!" );
        return false;
    }

    const VkPhysicalDeviceProperties& pProps = rg.physicalDevice.GetMetadata()->properties;
    LOG( "Using device: '%s', Vulkan Version: %u.%u.%u", pProps.deviceName, VK_VERSION_MAJOR( pProps.apiVersion ),
        VK_VERSION_MINOR( pProps.apiVersion ), VK_VERSION_PATCH( pProps.apiVersion ) );

    DebugMarker::Init( rg.instance );

    if ( !rg.device.Create( rg.physicalDevice, rg.instance ) )
    {
        LOG_ERR( "Failed to create logical device" );
        return false;
    }
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( rg.device, "Primary" );
    PG_DEBUG_MARKER_SET_QUEUE_NAME( rg.device.GetQueue( QueueType::GRAPHICS ).queue, "Primary GCT" );
    if ( rg.device.HasDedicatedTransferQueue() )
        PG_DEBUG_MARKER_SET_QUEUE_NAME( rg.device.GetQueue( QueueType::TRANSFER ).queue, "Dedicated Transfer" );

    LoadVulkanExtensions( rg.device );

    InitGlobalDescriptorData();
    BindlessManager::Init();
    PipelineManager::Init();

    PG_DEBUG_MARKER_SET_INSTANCE_NAME( rg.instance, "Primary" );
    PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( rg.physicalDevice, rg.physicalDevice.GetName() );

    if ( !headless )
    {
        if ( !rg.swapchain.Create( displayWidth, displayHeight ) )
            return false;
        PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( rg.swapchain, "Primary" );
        rg.displayWidth  = rg.swapchain.GetWidth();
        rg.displayHeight = rg.swapchain.GetHeight();
    }

    InitCommandObjects();
    InitSyncObjects();

    return true;
}

void R_Shutdown()
{
    rg.immediateCmdPool.Free();
    rg.immediateFence.Free();
    if ( !rg.headless )
    {
        for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
        {
            FrameData& frameData = rg.frameData[i];
            frameData.cmdPool.Free();
            frameData.acquireImageSemaphore.Free();
            frameData.renderingCompleteFence.Free();
        }

        rg.swapchain.Free();
        vkDestroySurfaceKHR( rg.instance, rg.surface, nullptr );
    }
    PipelineManager::Shutdown();
    BindlessManager::Shutdown();
    FreeGlobalDescriptorData();
    rg.device.Free();
#if USING( DEVELOPMENT_BUILD )
    if ( rg.debugUtilsEnabled )
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( rg.instance, "vkDestroyDebugUtilsMessengerEXT" );
        if ( !func )
            LOG_ERR( "Failed to get vkDestroyDebugUtilsMessengerEXT somehow" );
        else
            func( rg.instance, s_debugMessenger, nullptr );
    }
#endif // #if USING( DEVELOPMENT_BUILD )
    vkDestroyInstance( rg.instance, nullptr );
}

} // namespace PG::Gfx
