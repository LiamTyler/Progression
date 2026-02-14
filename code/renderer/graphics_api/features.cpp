#include "features.hpp"
#include <array>

namespace PG::Gfx
{

bool PhysicalDeviceExtensions::QuerySupport( VkPhysicalDevice vkPhysicalDevice )
{
    uint32_t numDeviceExtensions = 0;
    VK_CHECK( vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &numDeviceExtensions, nullptr ) );

    std::vector<VkExtensionProperties> deviceExtensions( numDeviceExtensions );
    VK_CHECK( vkEnumerateDeviceExtensionProperties( vkPhysicalDevice, nullptr, &numDeviceExtensions, deviceExtensions.data() ) );

    extensionNameListSize = 0;
    memset( extensionsPresent, 0, sizeof( extensionsPresent ) );
    for ( u32 i = 0; i < numDeviceExtensions; ++i )
    {
        for ( u32 j = 0; j < COUNT; ++j )
        {
            if ( std::strcmp( extensionData[j].first, deviceExtensions[i].extensionName ) == 0 )
            {
                extensionsPresent[j]                       = true;
                extensionNameList[extensionNameListSize++] = extensionData[j].first;
            }
        }
    }

    allRequiredExtensionsPresent = true;
    for ( u32 i = 0; i < COUNT; ++i )
    {
        if ( extensionData[i].second == ExtClassification::REQUIRED && !extensionsPresent[i] )
            allRequiredExtensionsPresent = false;
    }

    return allRequiredExtensionsPresent;
}

void PhysicalDeviceExtensions::LogMissingExtensions() const
{
    for ( u32 i = 0; i < COUNT; ++i )
    {
        if ( extensionData[i].second == ExtClassification::REQUIRED && !extensionsPresent[i] )
        {
            LOG( "    Missing: %s", extensionData[i].first );
        }
    }
}

enum
{
    REQUIRED,
    IMPLICIT,
};

struct FeatureItem
{
    u32 offset;
    u32 status;
    const char* msg;

    constexpr FeatureItem( size_t inOffset, u32 inStatus, const char* inMsg )
        : offset( static_cast<u32>( inOffset ) ), status( inStatus ), msg( inMsg )
    {
    }
};

#define STR( x ) #x

#define FEAT( structName, memberName, classification ) \
    FeatureItem { offsetof( PhysicalDeviceFeatures, structName.memberName ), classification, STR( structName.memberName ) }

// clang-format off
static constexpr std::array DEVICE_FEATURES = std::array{
    FEAT( vulkan13Features, dynamicRendering, REQUIRED ),
    FEAT( vulkan13Features, synchronization2, REQUIRED ), // simplified attachment layouts added, among other features
    FEAT( vulkan13Features, maintenance4, REQUIRED ),
    
    FEAT( vulkan12Features, bufferDeviceAddress, REQUIRED ),
    FEAT( vulkan12Features, descriptorIndexing, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingPartiallyBound, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingVariableDescriptorCount, REQUIRED ),
    FEAT( vulkan12Features, runtimeDescriptorArray, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingSampledImageUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageImageUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageTexelBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingUniformBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingUniformTexelBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, shaderSampledImageArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageImageArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageTexelBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderUniformBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderUniformTexelBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, scalarBlockLayout, REQUIRED ),
    FEAT( vulkan12Features, hostQueryReset, REQUIRED ),
    FEAT( vulkan12Features, shaderInt8, REQUIRED ),
    FEAT( vulkan12Features, storageBuffer8BitAccess, REQUIRED ),

    FEAT( vulkan11Features, shaderDrawParameters, REQUIRED ),
    FEAT( vulkan11Features, storageBuffer16BitAccess, REQUIRED ),

    FEAT( features2.features, shaderInt16, REQUIRED ),
    FEAT( features2.features, shaderInt64, REQUIRED ),
    FEAT( features2.features, samplerAnisotropy, REQUIRED ), // Could support implicitly, if sampler::anisotropyEnable==False when this is missing

    FEAT( fragmentShaderBarycentricFeaturesKHR, fragmentShaderBarycentric, IMPLICIT ),

    FEAT( meshShaderFeatures, meshShader, REQUIRED ),
    FEAT( meshShaderFeatures, taskShader, IMPLICIT ),

#if USING( SHADER_DEBUG_PRINTF )
    // While these are all useful, I think the only usage for them currently is in shader debug printf
    // https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/debug_printf.md#limitations
    FEAT( vulkan12Features, timelineSemaphore, IMPLICIT ),
    FEAT( features2.features, fragmentStoresAndAtomics, IMPLICIT ),
    FEAT( features2.features, vertexPipelineStoresAndAtomics, IMPLICIT ),
#endif // #if USING( SHADER_DEBUG_PRINTF )
};
// clang-format on

#undef FEAT

void FillFeaturesStruct( PhysicalDeviceFeatures& physicalDeviceFeatures )
{
    u8* baseAddress = reinterpret_cast<u8*>( &physicalDeviceFeatures );
    for ( const auto& item : DEVICE_FEATURES )
    {
        VkBool32& value = *reinterpret_cast<VkBool32*>( baseAddress + item.offset );
        value           = VK_TRUE;
    }
}

void FillFeaturesToEnableStruct( const PhysicalDeviceFeatures& deviceSupportedFeatures, PhysicalDeviceFeatures& featuresToEnable )
{
    const u8* deviceSupportedBaseAddress = reinterpret_cast<const u8*>( &deviceSupportedFeatures );
    u8* toEnableBaseAddress              = reinterpret_cast<u8*>( &featuresToEnable );
    for ( const auto& item : DEVICE_FEATURES )
    {
        VkBool32 supported = *reinterpret_cast<const VkBool32*>( deviceSupportedBaseAddress + item.offset );
        if ( supported )
        {
            VkBool32& value = *reinterpret_cast<VkBool32*>( toEnableBaseAddress + item.offset );
            value           = VK_TRUE;
        }
    }
}

void PhysicalDeviceFeatures::Initialize( const PhysicalDeviceExtensions& extensions )
{
    void* chain = {};

    vulkan13Features.pNext = chain;
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    chain                  = static_cast<void*>( &vulkan13Features );

    vulkan12Features.pNext = chain;
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    chain                  = static_cast<void*>( &vulkan12Features );

    vulkan11Features.pNext = chain;
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    chain                  = static_cast<void*>( &vulkan11Features );

#if USING( PG_MUTABLE_DESCRIPTORS )
    mutableDescriptorFeaturesEXT.pNext = chain;
    mutableDescriptorFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
    chain                              = static_cast<void*>( &mutableDescriptorFeaturesEXT );
#endif // #if USING( PG_MUTABLE_DESCRIPTORS )

#if USING( PG_DESCRIPTOR_BUFFER )
    descriptorBufferFeaturesEXT.pNext = chain;
    descriptorBufferFeaturesEXT.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
    chain                             = static_cast<void*>( &descriptorBufferFeaturesEXT );
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

#if USING( DEVELOPMENT_BUILD )
    if ( extensions.extensionsPresent[extensions.FRAGMENT_SHADER_BARYCENTRIC] )
    {
        // for solid wireframe debug mode
        fragmentShaderBarycentricFeaturesKHR.pNext = chain;
        fragmentShaderBarycentricFeaturesKHR.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR;
        chain                                      = static_cast<void*>( &fragmentShaderBarycentricFeaturesKHR );
    }
#endif // #if USING( DEVELOPMENT_BUILD )

    if ( extensions.extensionsPresent[extensions.MESH_SHADER] )
    {
        meshShaderFeatures.pNext = chain;
        meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        chain                    = static_cast<void*>( &meshShaderFeatures );
    }

    features2.pNext = chain;
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
}

bool PhysicalDeviceFeatures::CheckSuitability() const
{
    const u8* baseAddress = reinterpret_cast<const u8*>( this );
    for ( const auto& item : DEVICE_FEATURES )
    {
        VkBool32 value = *reinterpret_cast<const VkBool32*>( baseAddress + item.offset );
        if ( item.status == REQUIRED && value == VK_FALSE )
            return false;
    }

    return true;
}

void PhysicalDeviceFeatures::LogMissingFeatures() const
{
    const u8* baseAddress = reinterpret_cast<const u8*>( this );
    for ( const auto& item : DEVICE_FEATURES )
    {
        VkBool32 value = *reinterpret_cast<const VkBool32*>( baseAddress + item.offset );
        if ( value == VK_FALSE )
        {
            if ( item.status == REQUIRED )
                LOG( "  Missing support for REQUIRED feature: %s", item.msg );
            else if ( item.status == IMPLICIT )
                LOG( "  Missing support for IMPLICIT feature: %s", item.msg );
        }
    }
}

const void* PhysicalDeviceFeatures::GetPointerForDeviceCreationPNext() const { return &features2; }

} // namespace PG::Gfx
