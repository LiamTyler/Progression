#include "renderer/r_init.hpp"
#include "core/window.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#include "renderer/r_texture_manager.hpp"
#include "shared/logger.hpp"
#include "vk-bootstrap/VkBootstrap.h"
#include <cstring>

#include <iostream>

VkDebugUtilsMessengerEXT s_debugMessenger;
VkDescriptorSetLayout s_emptyDescriptSetLayout;

namespace PG::Gfx
{

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
    PG_UNUSED( pUserData );

    std::string messageTypeString;
    switch ( messageType )
    {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return VK_FALSE;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: messageTypeString = "Validation"; break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: messageTypeString = "Performance"; break;
    default: messageTypeString = "Unknown";
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

static void InitCommandObjects()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        rg.frameData[i].cmdPool          = rg.device.NewCommandPool( COMMAND_POOL_RESET_COMMAND_BUFFER, "frame" + std::to_string( i ) );
        rg.frameData[i].primaryCmdBuffer = rg.frameData[i].cmdPool.NewCommandBuffer( "primary" + std::to_string( i ) );
    }

    rg.immediateCmdPool   = rg.device.NewCommandPool( COMMAND_POOL_TRANSIENT | COMMAND_POOL_RESET_COMMAND_BUFFER, "immediate" );
    rg.immediateCmdBuffer = rg.immediateCmdPool.NewCommandBuffer( "immediate" );
}

static void InitSyncObjects()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        std::string iStr                           = std::to_string( i );
        rg.frameData[i].swapchainSemaphore         = rg.device.NewSemaphore( "swapchainSemaphore" + iStr );
        rg.frameData[i].renderingCompleteSemaphore = rg.device.NewSemaphore( "renderingCompleteSemaphore" + iStr );
        rg.frameData[i].renderingCompleteFence     = rg.device.NewFence( true, "renderingCompleteFence" + iStr );
    }

    rg.immediateFence = rg.device.NewFence( false, "immediate" );
}

bool R_Init( bool headless, uint32_t displayWidth, uint32_t displayHeight )
{
    rg.currentFrameIdx = 0;

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.request_validation_layers( !USING( SHIP_BUILD ) )
                        .set_debug_callback( DebugCallback )
                        .require_api_version( 1, 3, 0 )
                        .build();

    if ( !inst_ret )
    {
        LOG_ERR( "Failed to create vulkan instance. Error: %s", inst_ret.error().message().c_str() );
        return false;
    }

    vkb::Instance vkb_inst = inst_ret.value();

    rg.instance      = vkb_inst.instance;
    s_debugMessenger = vkb_inst.debug_messenger;

    rg.surface = VK_NULL_HANDLE;
    if ( !headless )
    {
        VK_CHECK( glfwCreateWindowSurface( rg.instance, GetMainWindow()->GetGLFWHandle(), nullptr, &rg.surface ) );
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

    vkb::PhysicalDeviceSelector pDevSelector{ vkb_inst };
#if USING( PG_RTX )
    pDevSelector.add_required_extension( VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME );
#endif // #if USING( PG_RTX )

#if USING( DEBUG_BUILD ) // perf hit
    pDevSelector.add_required_extension( VK_EXT_ROBUSTNESS_2_EXTENSION_NAME );
#endif // #if USING( DEBUG_BUILD )

#if USING( PG_MUTABLE_DESCRIPTORS )
    pDevSelector.add_required_extension( VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME );
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableExt{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT };
    mutableExt.mutableDescriptorType = true;

    mutableExt.pNext = features12.pNext;
    features12PNextChain.push_back( &mutableExt );
#endif // #if USING( PG_MUTABLE_DESCRIPTORS )

#if USING( PG_DESCRIPTOR_BUFFER )
    pDevSelector.add_required_extension( VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME );
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferExt{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
    descriptorBufferExt.descriptorBuffer = USING( PG_DESCRIPTOR_BUFFER );
    // descriptorBufferExt.descriptorBufferImageLayoutIgnored = true;

    descriptorBufferExt.pNext = features12.pNext;
    features12.pNext          = &descriptorBufferExt;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
    meshShaderFeatures.meshShader = true;
    meshShaderFeatures.taskShader = true;
    pDevSelector.add_required_extension_features( meshShaderFeatures );

    pDevSelector.add_required_extension( VK_KHR_SPIRV_1_4_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_EXT_MESH_SHADER_EXTENSION_NAME );
    pDevSelector.add_required_extension( VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME );
    pDevSelector.set_minimum_version( 1, 3 );
    pDevSelector.require_present( !headless );
    pDevSelector.set_required_features_13( features13 );
    pDevSelector.set_required_features_12( features12 );

    if ( !headless )
        pDevSelector.set_surface( rg.surface );

    auto pDevSelectorRet = pDevSelector.select();
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

#if !USING( SHIP_BUILD )
    DebugMarker::Init( rg.instance );
#endif // #if !USING( SHIP_BUILD )

    vkb::DeviceBuilder device_builder{ vkbPhysicalDevice };
    // std::vector<vkb::CustomQueueDescription> queue_descriptions;
    // const auto& queue_families = vkbPhysicalDevice.get_queue_families();
    // VkQueueFlags qFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    // for ( uint32_t i = 0; i < static_cast<uint32_t>( queue_families.size() ); ++i )
    //{
    //     if ( queue_families[i].queueFlags & qFlags )
    //     {
    //         if (
    //         queue_descriptions.push_back( vkb::CustomQueueDescription( i, std::vector<float>( 1, 1.0f ) ) );
    //     }
    // }
    // device_builder.custom_queue_setup( queue_descriptions );

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
    TextureManager::Init();
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
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        FrameData& frameData = rg.frameData[i];
        frameData.cmdPool.Free();
        frameData.swapchainSemaphore.Free();
        frameData.renderingCompleteSemaphore.Free();
        frameData.renderingCompleteFence.Free();
    }
    rg.immediateCmdPool.Free();
    rg.immediateFence.Free();

    rg.swapchain.Free();
    vkDestroySurfaceKHR( rg.instance, rg.surface, nullptr );
    PipelineManager::Shutdown();
    TextureManager::Shutdown();
    FreeGlobalDescriptorData();
    rg.device.Free();
    vkb::destroy_debug_utils_messenger( rg.instance, s_debugMessenger );
    vkDestroyInstance( rg.instance, nullptr );
}

VkDescriptorSetLayout GetEmptyDescriptorSetLayout() { return s_emptyDescriptSetLayout; }

} // namespace PG::Gfx
