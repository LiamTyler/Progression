﻿#include "renderer/r_init.hpp"
#include "core/dvars.hpp"
#include "core/window.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#include "shared/logger.hpp"
#include "vk-bootstrap/VkBootstrap.h"
#include <cstring>
#include <iostream>
#ifdef PG_USE_SDL
#include "SDL3/SDL_vulkan.h"
#endif // #ifdef PG_USE_SDL

VkDebugUtilsMessengerEXT s_debugMessenger;

#define SHADER_DEBUG_PRINTF USE_IF( USING( DEVELOPMENT_BUILD ) )

namespace PG::Gfx
{

Dvar r_shaderDebugPrint( "r_shaderDebugPrint", false, "Enable/disable logging of shader debug printf messages" );

#if USING( DEVELOPMENT_BUILD )
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
    PG_UNUSED( pUserData );

#if USING( SHADER_DEBUG_PRINTF )
    if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT && pCallbackData->pMessageIdName )
    {
        if ( !strcmp( pCallbackData->pMessageIdName, "WARNING-DEBUG-PRINTF" ) )
        {
            if ( !r_shaderDebugPrint.GetBool() )
                return VK_FALSE;

            std::string_view msg = pCallbackData->pMessage;
            size_t idx           = msg.find( "vkQueueSubmit(): " );
            if ( idx != std::string_view::npos )
            {
                idx += 17; // length of "vkQueueSubmit(): "
            }
            else
            {
                idx = msg.find( "vkQueueSubmit2(): pSubmits[0] " );
                if ( idx != std::string_view::npos )
                    idx += 30; // length of "vkQueueSubmit2(): pSubmits[0] "
                else
                    idx = 0;
            }
            Logger_Log(
                LogSeverity::DEBUG, TerminalColorCode::YELLOW, TerminalEmphasisCode::NONE, "SHADER PRINTF: %s", msg.substr( idx ).data() );
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
        LOG_ERR( "Vulkan message type '%s': '%s'", messageTypeString, pCallbackData->pMessage );
    }
    else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        LOG_WARN( "Vulkan message type '%s': '%s'", messageTypeString, pCallbackData->pMessage );
    }
    else
    {
        if ( pCallbackData->messageIdNumber != 601872502 ) // just a message of which instance layers are enabled
            LOG( "Vulkan message type '%s': '%s'", messageTypeString, pCallbackData->pMessage );
    }

    return VK_FALSE;
}
#endif // #if USING( DEVELOPMENT_BUILD )

static void InitCommandObjects()
{
    CommandPoolCreateInfo cmdPoolCI;
    cmdPoolCI.flags     = COMMAND_POOL_RESET_COMMAND_BUFFER;
    cmdPoolCI.queueType = QueueType::GRAPHICS;

    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].cmdPool          = rg.device.NewCommandPool( cmdPoolCI, "frame" + std::to_string( i ) );
        rg.frameData[i].primaryCmdBuffer = rg.frameData[i].cmdPool.NewCommandBuffer( "primary" + std::to_string( i ) );
    }

    cmdPoolCI.flags |= COMMAND_POOL_TRANSIENT;
    rg.immediateCmdPool   = rg.device.NewCommandPool( cmdPoolCI, "immediate" );
    rg.immediateCmdBuffer = rg.immediateCmdPool.NewCommandBuffer( "immediate" );
}

static void InitSyncObjects()
{
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        std::string iStr                           = std::to_string( i );
        rg.frameData[i].swapchainSemaphore         = rg.device.NewSemaphore( "swapchainSemaphore" + iStr );
        rg.frameData[i].renderingCompleteSemaphore = rg.device.NewSemaphore( "renderingCompleteSemaphore" + iStr );
        rg.frameData[i].renderingCompleteFence     = rg.device.NewFence( true, "renderingCompleteFence" + iStr );
    }

    rg.immediateFence = rg.device.NewFence( false, "immediate" );
}

static vkb::Result<vkb::Instance> GetInstance()
{
    PGP_MANUAL_ZONEN( __tracyInstBuild, "Instance Builder" );
    vkb::InstanceBuilder builder;

#if USING( DEVELOPMENT_BUILD )
    builder.request_validation_layers( true );
    builder.set_debug_callback( DebugCallback );
    VkDebugUtilsMessageSeverityFlagsEXT debugMessageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#if USING( SHADER_DEBUG_PRINTF )
    builder.add_validation_feature_enable( VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT );
    debugMessageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#endif // #if USING( SHADER_DEBUG_PRINTF )
    builder.set_debug_messenger_severity( debugMessageSeverity );
#endif // #if USING( DEVELOPMENT_BUILD )

    builder.require_api_version( 1, 3, 0 );
    auto inst_ret = builder.build();
    PGP_MANUAL_ZONE_END( __tracyInstBuild );

    return inst_ret;
}

bool R_Init( bool headless, u32 displayWidth, u32 displayHeight )
{
    PGP_ZONE_SCOPEDN( "R_Init" );
    rg.currentFrameIdx = rg.totalFrameCount = 0;
    rg.headless                             = headless;

    auto instRet = GetInstance();
    if ( !instRet )
    {
        LOG_ERR( "Failed to create vulkan instance. Error: %s", instRet.error().message().c_str() );
        return false;
    }
    vkb::Instance vkb_inst = instRet.value();
    rg.instance            = vkb_inst.instance;
    s_debugMessenger       = vkb_inst.debug_messenger;

    rg.surface = VK_NULL_HANDLE;
    if ( !headless )
    {
        PGP_ZONE_SCOPEDN( "CreateSurfaceFromWindow" );
#ifdef PG_USE_SDL
        SDL_Vulkan_CreateSurface( GetMainWindow()->GetHandle(), rg.instance, nullptr, &rg.surface );
#else  // #ifdef PG_USE_SDL
        VK_CHECK( glfwCreateWindowSurface( rg.instance, GetMainWindow()->GetHandle(), nullptr, &rg.surface ) );
#endif // #else // #ifdef PG_USE_SDL
    }

    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;
    features13.maintenance4     = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress                                = true;
    features12.descriptorIndexing                                 = true;
    features12.descriptorBindingPartiallyBound                    = true;
    features12.descriptorBindingVariableDescriptorCount           = true;
    features12.runtimeDescriptorArray                             = true;
    features12.descriptorBindingSampledImageUpdateAfterBind       = true;
    features12.descriptorBindingStorageBufferUpdateAfterBind      = true;
    features12.descriptorBindingStorageImageUpdateAfterBind       = true;
    features12.descriptorBindingStorageTexelBufferUpdateAfterBind = true;
    features12.descriptorBindingUniformBufferUpdateAfterBind      = true;
    features12.descriptorBindingUniformTexelBufferUpdateAfterBind = true;
    features12.shaderSampledImageArrayNonUniformIndexing          = true;
    features12.shaderStorageBufferArrayNonUniformIndexing         = true;
    features12.shaderStorageImageArrayNonUniformIndexing          = true;
    features12.shaderStorageTexelBufferArrayNonUniformIndexing    = true;
    features12.shaderUniformBufferArrayNonUniformIndexing         = true;
    features12.shaderUniformTexelBufferArrayNonUniformIndexing    = true;
    features12.scalarBlockLayout                                  = true;
    features12.hostQueryReset                                     = true;
    features12.shaderInt8                                         = true;
    features12.timelineSemaphore                                  = true;

    VkPhysicalDeviceVulkan11Features features11{};
    features11.shaderDrawParameters = true; // for drawid in shader (gl_DrawIDARB)

    VkPhysicalDeviceFeatures features{};
    features.shaderInt64       = true;
    features.samplerAnisotropy = true;

#define ADD_PNEXT_FEATURES12( extStruct ) \
    extStruct.pNext  = features12.pNext;  \
    features12.pNext = &extStruct;

    vkb::PhysicalDeviceSelector pDevSelector{ vkb_inst };
#if USING( PG_RTX )
    pDevSelector.add_required_extension( VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME );
#endif // #if USING( PG_RTX )

#if USING( PG_MUTABLE_DESCRIPTORS )
    pDevSelector.add_required_extension( VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME );
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableExt{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
    mutableExt.mutableDescriptorType = true;
    ADD_PNEXT_FEATURES12( mutableExt );
#endif // #if USING( PG_MUTABLE_DESCRIPTORS )

#if USING( PG_DESCRIPTOR_BUFFER )
    pDevSelector.add_required_extension( VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME );
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferExt{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
    descriptorBufferExt.descriptorBuffer = USING( PG_DESCRIPTOR_BUFFER );
    // descriptorBufferExt.descriptorBufferImageLayoutIgnored = true;
    ADD_PNEXT_FEATURES12( descriptorBufferExt );
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

#if USING( DEVELOPMENT_BUILD )
    // for solid wireframe debug mode
    pDevSelector.add_required_extension( VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME );
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR baryFeaturesKHR = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR };
    baryFeaturesKHR.fragmentShaderBarycentric = true;
    // ADD_PNEXT_FEATURES12( baryFeaturesKHR );
    pDevSelector.add_required_extension_features( baryFeaturesKHR );
#endif // #if USING( DEVELOPMENT_BUILD )

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshShaderFeatures.meshShader = true;
    meshShaderFeatures.taskShader = true;
    pDevSelector.add_required_extension_features( meshShaderFeatures );
    pDevSelector.add_required_extension( VK_EXT_MESH_SHADER_EXTENSION_NAME );

    pDevSelector.add_required_extension( VK_KHR_SPIRV_1_4_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME );
#if USING( SHADER_DEBUG_PRINTF )
    pDevSelector.add_required_extension( VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME );
#endif // #if USING( SHADER_DEBUG_PRINTF )
    pDevSelector.set_minimum_version( 1, 3 );
    pDevSelector.require_present( !rg.headless );
    pDevSelector.set_required_features_13( features13 );
    pDevSelector.set_required_features_12( features12 );
    pDevSelector.set_required_features_11( features11 );
    pDevSelector.set_required_features( features );

    if ( !rg.headless )
        pDevSelector.set_surface( rg.surface );

    PGP_MANUAL_ZONEN( __prof__pDevSelect, "Physical Device Selection" );
    auto pDevSelectorRet = pDevSelector.select();
    PGP_MANUAL_ZONE_END( __prof__pDevSelect );
    if ( !pDevSelectorRet )
    {
        LOG_ERR( "No compatible physical device found! " );
        return false;
    }

    vkb::PhysicalDevice vkbPhysicalDevice = pDevSelectorRet.value();
    rg.physicalDevice.Create( vkbPhysicalDevice.physical_device );
    auto pDevProperties = rg.physicalDevice.GetProperties();
    LOG( "Using device: '%s', Vulkan Version: %u.%u.%u", rg.physicalDevice.GetName().c_str(), pDevProperties.apiVersionMajor,
        pDevProperties.apiVersionMinor, pDevProperties.apiVersionPatch );

#if USING( DEVELOPMENT_BUILD )
    DebugMarker::Init( rg.instance );
#endif // #if USING( DEVELOPMENT_BUILD )

    vkb::DeviceBuilder device_builder{ vkbPhysicalDevice };

    auto dev_ret = device_builder.build();
    if ( !dev_ret )
    {
        LOG_ERR( "Failed to create logical device!" );
        return false;
    }
    vkb::Device vkb_device = dev_ret.value();
    if ( !rg.device.Create( vkb_device ) )
    {
        LOG_ERR( "Failed to create logical device" );
        return false;
    }
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
    for ( i32 i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& frameData = rg.frameData[i];
        frameData.cmdPool.Free();
        frameData.swapchainSemaphore.Free();
        frameData.renderingCompleteSemaphore.Free();
        frameData.renderingCompleteFence.Free();
    }
    rg.immediateCmdPool.Free();
    rg.immediateFence.Free();

    LOG( "Freeing swapchain..." );
    rg.swapchain.Free();
    LOG( "Freeing surface..." );
    vkDestroySurfaceKHR( rg.instance, rg.surface, nullptr );
    LOG( "Freeing pipelines..." );
    PipelineManager::Shutdown();
    BindlessManager::Shutdown();
    FreeGlobalDescriptorData();
    rg.device.Free();
    vkb::destroy_debug_utils_messenger( rg.instance, s_debugMessenger );
    vkDestroyInstance( rg.instance, nullptr );
}

} // namespace PG::Gfx
