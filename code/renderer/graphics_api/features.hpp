#pragma once

#include "renderer/vulkan.hpp"
#include "shared/core_defines.hpp"

namespace PG::Gfx
{

enum class ExtClassification : u8
{
    REQUIRED = 0,
    IMPLICIT = 1,
};

// Based off of Daxa's feature system
struct PhysicalDeviceExtensions
{
    enum Extension
    {
        SWAPCHAIN,
        MESH_SHADER,
        SPIRV_1_4,                   // ?
        HOST_QUERY_RESET,            // ?
        SCALAR_BLOCK_LAYOUT,         // qualifier for ssbos/ubos/push constants to allow packed members, like c-style structs
        SHADER_NON_SEMANTIC_INFO,    // for shader printf
        FRAGMENT_SHADER_BARYCENTRIC, // used in debug wireframe shader
        // ACCELERATION_STRUCTURE,      // rt
        // RAY_TRACING_PIPELINE,        // rt
        // DEFERRED_HOST_OPERATIONS,    // rt?

        // MUTABLE_DESCRIPTOR_TYPE,
        // DESCRIPTOR_BUFFER,

        COUNT,
    };
    static constexpr std::pair<char const*, ExtClassification> extensionData[COUNT] = {
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME,                   ExtClassification::IMPLICIT},
        {VK_EXT_MESH_SHADER_EXTENSION_NAME,                 ExtClassification::REQUIRED},
        {VK_KHR_SPIRV_1_4_EXTENSION_NAME,                   ExtClassification::REQUIRED},
        {VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,            ExtClassification::REQUIRED},
        {VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,         ExtClassification::REQUIRED},
        {VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,    ExtClassification::IMPLICIT},
        {VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, ExtClassification::IMPLICIT},

        // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        // VK_EXT_MUTABLE_DESCRIPTOR_TYPE_EXTENSION_NAME,
        // VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    };
    char const* extensionNameList[COUNT] = {};
    u32 extensionNameListSize            = {};
    bool extensionsPresent[COUNT]        = {};
    bool allRequiredExtensionsPresent    = false;

    // returns whether all requred extensions are available for this device or not
    bool QuerySupport( VkPhysicalDevice vkPhysicalDevice );
    void LogMissingExtensions() const;
};

struct PhysicalDeviceFeatures
{
    VkPhysicalDeviceVulkan13Features vulkan13Features                                         = {};
    VkPhysicalDeviceVulkan12Features vulkan12Features                                         = {};
    VkPhysicalDeviceVulkan11Features vulkan11Features                                         = {};
    VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeaturesKHR = {};
    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorFeaturesEXT             = {};
    VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeaturesEXT                   = {};
    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures                                  = {};
    VkPhysicalDeviceFeatures2 features2                                                       = {};

    void Initialize( const PhysicalDeviceExtensions& extensions );
    bool CheckSuitability() const;
    void LogMissingFeatures() const;
    const void* GetPointerForDeviceCreationPNext() const;
};

void FillFeaturesStruct( PhysicalDeviceFeatures& physicalDeviceFeatures );
void FillFeaturesToEnableStruct( const PhysicalDeviceFeatures& deviceSupportedFeatures, PhysicalDeviceFeatures& featuresToEnable );

} // namespace PG::Gfx
