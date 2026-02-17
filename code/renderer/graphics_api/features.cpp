#include "features.hpp"
#define SPV_ENABLE_UTILITY_CODE
#include "spirv_cross/spirv.hpp"
#include <array>
#include <span>

#define STR( x ) #x

namespace PG::Gfx
{

struct ExtensionData
{
    const char* name;
    ExtOrFeatClassification classification;
    constexpr ExtensionData( const char* inName, ExtOrFeatClassification c ) : name( inName ), classification( c ) {}
};

static constexpr ExtensionData DEVICE_EXTENSIONS[PhysicalDeviceExtensions::COUNT] = {
    {"",                                                ExtOrFeatClassification::DISABLED}, // placeholder
    {VK_KHR_SWAPCHAIN_EXTENSION_NAME,                   ExtOrFeatClassification::IMPLICIT},
    {VK_EXT_MESH_SHADER_EXTENSION_NAME,                 ExtOrFeatClassification::REQUIRED},
    {VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,      ExtOrFeatClassification::REQUIRED},
    {VK_KHR_SPIRV_1_4_EXTENSION_NAME,                   ExtOrFeatClassification::REQUIRED},
    {VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,            ExtOrFeatClassification::REQUIRED},
    {VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,         ExtOrFeatClassification::REQUIRED},
    {VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,    ExtOrFeatClassification::IMPLICIT},
    {VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, ExtOrFeatClassification::DISABLED},
};

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
            if ( DEVICE_EXTENSIONS[j].classification == ExtOrFeatClassification::DISABLED )
                continue;

            if ( std::strcmp( DEVICE_EXTENSIONS[j].name, deviceExtensions[i].extensionName ) == 0 )
            {
                extensionsPresent[j]                       = true;
                extensionNameList[extensionNameListSize++] = DEVICE_EXTENSIONS[j].name;
            }
        }
    }

    allRequiredExtensionsPresent = true;
    for ( u32 i = 0; i < COUNT; ++i )
    {
        if ( DEVICE_EXTENSIONS[i].classification == ExtOrFeatClassification::REQUIRED && !extensionsPresent[i] )
            allRequiredExtensionsPresent = false;
    }

    return allRequiredExtensionsPresent;
}

void PhysicalDeviceExtensions::LogMissingExtensions() const
{
    for ( u32 i = 0; i < COUNT; ++i )
    {
        if ( DEVICE_EXTENSIONS[i].classification == ExtOrFeatClassification::REQUIRED && !extensionsPresent[i] )
        {
            LOG( "    Missing: %s", DEVICE_EXTENSIONS[i].name );
        }
    }
}

struct FeatureItem
{
    u32 offset;
    ExtOrFeatClassification classification;
    const char* msg;

    constexpr FeatureItem( size_t inOffset, ExtOrFeatClassification c, const char* inMsg )
        : offset( static_cast<u32>( inOffset ) ), classification( c ), msg( inMsg )
    {
    }
};

#define FEAT( structName, memberName, classification )                                                                                   \
    FeatureItem                                                                                                                          \
    {                                                                                                                                    \
        offsetof( PhysicalDeviceFeatures, structName.memberName ), ExtOrFeatClassification::classification, STR( structName.memberName ) \
    }

// clang-format off
static constexpr std::array DEVICE_FEATURES = std::array{
    FEAT( vulkan13Features, dynamicRendering, REQUIRED ),
    FEAT( vulkan13Features, synchronization2, REQUIRED ), // simplified attachment layouts added, among other features
    FEAT( vulkan13Features, maintenance4, REQUIRED ),
    
    FEAT( vulkan12Features, bufferDeviceAddress, REQUIRED ),

    // bindless
    FEAT( vulkan12Features, descriptorIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderInputAttachmentArrayDynamicIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderUniformTexelBufferArrayDynamicIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageTexelBufferArrayDynamicIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderUniformBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderSampledImageArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageImageArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderInputAttachmentArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderUniformTexelBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, shaderStorageTexelBufferArrayNonUniformIndexing, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingUniformBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingSampledImageUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageImageUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingUniformTexelBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingStorageTexelBufferUpdateAfterBind, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingUpdateUnusedWhilePending, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingPartiallyBound, REQUIRED ),
    FEAT( vulkan12Features, descriptorBindingVariableDescriptorCount, REQUIRED ),
    FEAT( vulkan12Features, runtimeDescriptorArray, REQUIRED ),

    FEAT( features2.features, shaderStorageImageReadWithoutFormat, REQUIRED ), // post process compute shader
    FEAT( features2.features, shaderStorageImageWriteWithoutFormat, REQUIRED ), // post process compute shader

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
    for ( const FeatureItem& item : DEVICE_FEATURES )
    {
        VkBool32 value = *reinterpret_cast<const VkBool32*>( baseAddress + item.offset );
        if ( item.classification == ExtOrFeatClassification::REQUIRED && value == VK_FALSE )
            return false;
    }

    return true;
}

void PhysicalDeviceFeatures::LogMissingFeatures() const
{
    const u8* baseAddress = reinterpret_cast<const u8*>( this );
    for ( const FeatureItem& item : DEVICE_FEATURES )
    {
        VkBool32 value = *reinterpret_cast<const VkBool32*>( baseAddress + item.offset );
        if ( value == VK_FALSE )
        {
            if ( item.classification == ExtOrFeatClassification::REQUIRED )
                LOG( "  Missing support for REQUIRED feature: %s", item.msg );
            else if ( item.classification == ExtOrFeatClassification::IMPLICIT )
                LOG( "  Missing support for IMPLICIT feature: %s", item.msg );
        }
    }
}

const void* PhysicalDeviceFeatures::GetPointerForDeviceCreationPNext() const { return &features2; }

static constexpr u32 NO_IMPLICIT_VERSION = ~0u;
struct SpirvExtension
{
    const char* name;
    u32 dependentVersion;
    PhysicalDeviceExtensions::Extension dependentDeviceExtension;
};

// https://docs.vulkan.org/spec/latest/appendices/spirvenv.html#spirvenv-extensions
static constexpr SpirvExtension SPIRV_EXTENSIONS[] = {
    { "SPV_EXT_mesh_shader", NO_IMPLICIT_VERSION, PhysicalDeviceExtensions::MESH_SHADER },
    { "SPV_KHR_fragment_shader_barycentric", NO_IMPLICIT_VERSION, PhysicalDeviceExtensions::FRAGMENT_SHADER_BARYCENTRIC },
    { "SPV_KHR_non_semantic_info", VK_MAKE_API_VERSION( 0, 1, 3, 0 ), PhysicalDeviceExtensions::SHADER_NON_SEMANTIC_INFO },
    { "SPV_KHR_physical_storage_buffer", VK_MAKE_API_VERSION( 0, 1, 2, 0 ), PhysicalDeviceExtensions::NONE },
    { "SPV_KHR_shader_draw_parameters", VK_MAKE_API_VERSION( 0, 1, 1, 0 ), PhysicalDeviceExtensions::SHADER_DRAW_PARAMETERS },
};

bool IsSpirvExtensionSupported( const PhysicalDeviceExtensions& deviceExtensions, u32 apiVersion, const std::string& spirvExtension )
{
    for ( const SpirvExtension& e : SPIRV_EXTENSIONS )
    {
        if ( !strcmp( e.name, spirvExtension.c_str() ) )
        {
            if ( e.dependentVersion != NO_IMPLICIT_VERSION )
                return apiVersion >= e.dependentVersion;
            return deviceExtensions.extensionsPresent[e.dependentDeviceExtension];
        }
    }

    LOG_WARN( "Unknown Spirv Extension: %s", spirvExtension.c_str() );
    return false;
}

struct SpirvCapability
{
    spv::Capability spvCapability;
    u32 dependentVersion;
    std::span<const u32> dependentFeatures;
};

#define SFEAT( structName, memberName ) ( u32 ) offsetof( PhysicalDeviceFeatures, structName.memberName )
// clang-format off
static constexpr std::array SPV_CAPS_Geometry = std::array{ SFEAT( features2.features, geometryShader ) };
static constexpr std::array SPV_CAPS_Tessellation = std::array{ SFEAT( features2.features, tessellationShader ) };
static constexpr std::array SPV_CAPS_Float64 = std::array{ SFEAT( features2.features, shaderFloat64 ) };
static constexpr std::array SPV_CAPS_Int64 = std::array{ SFEAT( features2.features, shaderInt64 ) };
static constexpr std::array SPV_CAPS_Int16 = std::array{ SFEAT( features2.features, shaderInt16 ) };
static constexpr std::array SPV_CAPS_Int8 = std::array{ SFEAT( vulkan12Features, shaderInt8 ) };
static constexpr std::array SPV_CAPS_RuntimeDescriptorArray = std::array{ SFEAT( vulkan12Features, runtimeDescriptorArray ) };
static constexpr std::array SPV_CAPS_PhysicalStorageBufferAddresses = std::array{ SFEAT( vulkan12Features, bufferDeviceAddress ) };
static constexpr std::array SPV_CAPS_DrawParameters = std::array{ SFEAT( vulkan11Features, shaderDrawParameters ) };
static constexpr std::array SPV_CAPS_StorageBuffer8BitAccess = std::array{ SFEAT( vulkan12Features, storageBuffer8BitAccess ) };
static constexpr std::array SPV_CAPS_StorageBuffer16BitAccess = std::array{ SFEAT( vulkan11Features, storageBuffer16BitAccess ) };
static constexpr std::array SPV_CAPS_UniformAndStorageBuffer16BitAccess = std::array{ SFEAT( vulkan11Features, uniformAndStorageBuffer16BitAccess ) };
static constexpr std::array SPV_CAPS_FragmentBarycentricKHR = std::array{ SFEAT( fragmentShaderBarycentricFeaturesKHR, fragmentShaderBarycentric ) };
static constexpr std::array SPV_CAPS_StorageImageReadWithoutFormat = std::array{ SFEAT( features2.features, shaderStorageImageReadWithoutFormat ) };
static constexpr std::array SPV_CAPS_StorageImageWriteWithoutFormat = std::array{ SFEAT( features2.features, shaderStorageImageWriteWithoutFormat ) };
static constexpr std::array SPV_CAPS_MeshShading = std::array{ SFEAT( meshShaderFeatures, meshShader ) };
// I don't actually know which of these the spirv necessarily refers to...
static constexpr std::array SPV_CAPS_ShaderNonUniform = std::array{
    SFEAT( vulkan12Features, descriptorIndexing ),
    SFEAT( vulkan12Features, shaderInputAttachmentArrayDynamicIndexing ),
    SFEAT( vulkan12Features, shaderUniformTexelBufferArrayDynamicIndexing ),
    SFEAT( vulkan12Features, shaderStorageTexelBufferArrayDynamicIndexing ),
    SFEAT( vulkan12Features, shaderUniformBufferArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderSampledImageArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderStorageBufferArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderStorageImageArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderInputAttachmentArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderUniformTexelBufferArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, shaderStorageTexelBufferArrayNonUniformIndexing ),
    SFEAT( vulkan12Features, runtimeDescriptorArray ),
};
#undef SFEAT

// https://docs.vulkan.org/spec/latest/appendices/spirvenv.html#spirvenv-capabilities
static constexpr std::array SPIRV_CAPABILITIES = std::array{
    SpirvCapability{ spv::CapabilityMatrix, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityShader, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityInputAttachment, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilitySampled1D, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityImage1D, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilitySampledBuffer, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityImageBuffer, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityImageQuery, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    SpirvCapability{ spv::CapabilityDerivativeControl, VK_MAKE_API_VERSION( 0, 1, 0, 0 ) },
    
    SpirvCapability{ spv::CapabilityGeometry, NO_IMPLICIT_VERSION, SPV_CAPS_Geometry },
    SpirvCapability{ spv::CapabilityTessellation, NO_IMPLICIT_VERSION, SPV_CAPS_Tessellation },
    SpirvCapability{ spv::CapabilityFloat64, NO_IMPLICIT_VERSION, SPV_CAPS_Float64 },
    SpirvCapability{ spv::CapabilityInt64, NO_IMPLICIT_VERSION, SPV_CAPS_Int64 },
    SpirvCapability{ spv::CapabilityInt16, NO_IMPLICIT_VERSION, SPV_CAPS_Int16 },
    SpirvCapability{ spv::CapabilityInt8, NO_IMPLICIT_VERSION, SPV_CAPS_Int8 },
    SpirvCapability{ spv::CapabilityRuntimeDescriptorArray, NO_IMPLICIT_VERSION, SPV_CAPS_RuntimeDescriptorArray },
    SpirvCapability{ spv::CapabilityPhysicalStorageBufferAddresses, NO_IMPLICIT_VERSION, SPV_CAPS_PhysicalStorageBufferAddresses },
    SpirvCapability{ spv::CapabilityDrawParameters, NO_IMPLICIT_VERSION, SPV_CAPS_DrawParameters },
    SpirvCapability{ spv::CapabilityStorageBuffer8BitAccess, NO_IMPLICIT_VERSION, SPV_CAPS_StorageBuffer8BitAccess },
    SpirvCapability{ spv::CapabilityStorageBuffer16BitAccess, NO_IMPLICIT_VERSION, SPV_CAPS_StorageBuffer16BitAccess },
    SpirvCapability{ spv::CapabilityUniformAndStorageBuffer16BitAccess, NO_IMPLICIT_VERSION, SPV_CAPS_UniformAndStorageBuffer16BitAccess },
    SpirvCapability{ spv::CapabilityFragmentBarycentricKHR, NO_IMPLICIT_VERSION, SPV_CAPS_FragmentBarycentricKHR },
    SpirvCapability{ spv::CapabilityStorageImageReadWithoutFormat, NO_IMPLICIT_VERSION, SPV_CAPS_StorageImageReadWithoutFormat },
    SpirvCapability{ spv::CapabilityStorageImageWriteWithoutFormat, NO_IMPLICIT_VERSION, SPV_CAPS_StorageImageWriteWithoutFormat },
    SpirvCapability{ spv::CapabilityMeshShadingEXT, NO_IMPLICIT_VERSION, SPV_CAPS_MeshShading },
    SpirvCapability{ spv::CapabilityShaderNonUniform, NO_IMPLICIT_VERSION, SPV_CAPS_ShaderNonUniform },
};
// clang-format on

bool IsSpirvCapabilitySupported( const PhysicalDeviceFeatures& deviceExtensions, u32 apiVersion, i32 queryCapability )
{
    for ( const SpirvCapability& cap : SPIRV_CAPABILITIES )
    {
        if ( cap.spvCapability == queryCapability )
        {
            if ( cap.dependentVersion != NO_IMPLICIT_VERSION )
                return apiVersion >= cap.dependentVersion;
            const u8* baseAddress = reinterpret_cast<const u8*>( &deviceExtensions );
            for ( u32 offset : cap.dependentFeatures )
            {
                VkBool32 value = *reinterpret_cast<const VkBool32*>( baseAddress + offset );
                if ( value == VK_FALSE )
                    return false;
            }

            return true;
        }
    }

    LOG_WARN( "Unknown Spirv Capability: %d %s", queryCapability, spv::CapabilityToString( (spv::Capability)queryCapability ) );
    return false;
}

} // namespace PG::Gfx
