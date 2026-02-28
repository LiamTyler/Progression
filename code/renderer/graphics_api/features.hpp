#pragma once

#include "renderer/vulkan.hpp"
#include "shared/core_defines.hpp"

namespace PG::Gfx
{

enum class ExtOrFeatClassification : u8
{
    DISABLED = 0,
    REQUIRED = 1,
    IMPLICIT = 2,
};

// Based off of Daxa's feature system
struct PhysicalDeviceExtensions
{
    enum Extension
    {
        NONE, // just used as a placeholder tag for extensions that were promoted to core
        SWAPCHAIN,
        MESH_SHADER,
        SHADER_DRAW_PARAMETERS,
        SPIRV_1_4,                   // ?
        HOST_QUERY_RESET,            // ?
        SCALAR_BLOCK_LAYOUT,         // qualifier for ssbos/ubos/push constants to allow packed members, like c-style structs
        SHADER_NON_SEMANTIC_INFO,    // for shader printf
        FRAGMENT_SHADER_BARYCENTRIC, // used in debug wireframe shader

        // MUTABLE_DESCRIPTOR_TYPE,
        // DESCRIPTOR_BUFFER,

        COUNT,
    };

    char const* extensionNameList[COUNT] = {};
    u32 extensionNameListSize            = {};
    bool extensionsPresent[COUNT]        = {};
    bool allRequiredExtensionsPresent    = false;

    static void UpdateClassifications( bool headless );

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
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures                   = {};
    VkPhysicalDeviceFeatures2 features2                                                       = {};

    void Initialize( const PhysicalDeviceExtensions& extensions );
    bool CheckSuitability() const;
    void LogMissingFeatures() const;
    const void* GetPointerForDeviceCreationPNext() const;
};

void FillFeaturesToEnableStruct( const PhysicalDeviceFeatures& deviceSupportedFeatures, PhysicalDeviceFeatures& featuresToEnable );

bool IsSpirvExtensionSupported( const PhysicalDeviceExtensions& deviceExtensions, u32 apiVersion, const std::string& spirvExtension );
bool IsSpirvCapabilitySupported( const PhysicalDeviceFeatures& deviceExtensions, u32 apiVersion, i32 queryCapability );

} // namespace PG::Gfx
