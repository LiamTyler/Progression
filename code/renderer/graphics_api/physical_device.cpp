#include "physical_device.hpp"
#include "renderer/r_globals.hpp"
#include "shared/logger.hpp"
#include <algorithm>

static bool QueueSupportsTimestamps( VkQueueFamilyProperties q, VkQueueFlags flags )
{
#if USING( PG_GPU_PROFILING )
    if ( flags & VK_QUEUE_GRAPHICS_BIT || flags & VK_QUEUE_COMPUTE_BIT )
        return q.timestampValidBits > 0;
    else
        return true;
#else  // #if USING( PG_GPU_PROFILING )
    return true;
#endif // #else // #if USING( PG_GPU_PROFILING )
}

static uint32_t FindQueueFamily( bool headless, VkPhysicalDevice device, VkSurfaceKHR surface, VkQueueFlags flags )
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );
    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    // search for dedicated queue first
    for ( uint32_t i = 0; i < queueFamilyCount; ++i )
    {
        const VkQueueFamilyProperties& props = queueFamilies[i];
        if ( props.queueCount > 0 )
        {
            if ( ( props.queueFlags & flags ) == flags && QueueSupportsTimestamps( props, flags ) )
            {
                if ( !headless && ( flags & VK_QUEUE_GRAPHICS_BIT ) )
                {
                    VkBool32 presentSupport = false;
                    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport ) );
                    if ( presentSupport )
                        return i;
                }
                else
                {
                    return i;
                }
            }
        }
    }

    // if no dedicated queue found, look for any that has at least the requested flags
    for ( uint32_t i = 0; i < queueFamilyCount; ++i )
    {
        const VkQueueFamilyProperties& props = queueFamilies[i];
        if ( props.queueCount > 0 )
        {
            if ( props.queueFlags & flags && QueueSupportsTimestamps( props, flags ) )
            {
                if ( !headless && ( flags & VK_QUEUE_GRAPHICS_BIT ) )
                {
                    VkBool32 presentSupport = false;
                    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport ) );
                    if ( presentSupport )
                        return i;
                }
                else
                {
                    return i;
                }
            }
        }
    }

    // no family found
    return ~0u;
}

static bool HasSwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;

    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &capabilities ) );

    uint32_t formatCount;
    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr ) );

    if ( formatCount != 0 )
    {
        formats.resize( formatCount );
        VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, formats.data() ) );
    }

    uint32_t presentModeCount;
    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr ) );

    if ( presentModeCount != 0 )
    {
        presentModes.resize( presentModeCount );
        VK_CHECK_RESULT( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, presentModes.data() ) );
    }

    return !formats.empty() && !presentModes.empty();
}

namespace PG
{
namespace Gfx
{

static PhysicalDeviceProperties GetDeviceProperties( VkPhysicalDevice physicalDevice )
{
    VkPhysicalDeviceProperties vkProperties;
    vkGetPhysicalDeviceProperties( physicalDevice, &vkProperties );
    PhysicalDeviceProperties p = {};
    p.name                     = vkProperties.deviceName;
    p.apiVersionMajor          = VK_VERSION_MAJOR( vkProperties.apiVersion );
    p.apiVersionMinor          = VK_VERSION_MINOR( vkProperties.apiVersion );
    p.apiVersionPatch          = VK_VERSION_PATCH( vkProperties.apiVersion );
    p.isDiscrete               = vkProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    p.nanosecondsPerTick       = vkProperties.limits.timestampPeriod;

    return p;
}

static PhysicalDeviceFeatures GetDeviceFeatures( VkPhysicalDevice physicalDevice )
{
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
    VkPhysicalDeviceRobustness2FeaturesEXT vkFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT };
    VkPhysicalDeviceFeatures2 deviceFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };

    deviceFeatures2.pNext = MakePNextChain(
        { &bufferDeviceAddressFeatures, &rayTracingPipelineFeatures, &accelerationStructureFeatures, &indexingFeatures, &vkFeatures2 } );
    vkGetPhysicalDeviceFeatures2( physicalDevice, &deviceFeatures2 );

    PhysicalDeviceFeatures f = {};
    f.anisotropy             = deviceFeatures2.features.samplerAnisotropy;
    f.nullDescriptors        = vkFeatures2.nullDescriptor;
    f.bindless               = indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray &&
                 indexingFeatures.descriptorBindingSampledImageUpdateAfterBind &&
                 indexingFeatures.shaderSampledImageArrayNonUniformIndexing;
    f.raytracing = bufferDeviceAddressFeatures.bufferDeviceAddress && rayTracingPipelineFeatures.rayTracingPipeline &&
                   accelerationStructureFeatures.accelerationStructure;

    return f;
}

PhysicalDevice::PhysicalDevice( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface, bool headless )
{
    m_handle     = vkPhysicalDevice;
    m_properties = GetDeviceProperties( vkPhysicalDevice );
    m_features   = GetDeviceFeatures( vkPhysicalDevice );
    vkGetPhysicalDeviceMemoryProperties( m_handle, &m_memProperties );
    m_GCTQueueFamily = FindQueueFamily( headless, m_handle, surface, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT );
    m_graphicsFamily = FindQueueFamily( headless, m_handle, surface, VK_QUEUE_GRAPHICS_BIT );
    m_computeFamily  = FindQueueFamily( headless, m_handle, surface, VK_QUEUE_COMPUTE_BIT );
    m_transferFamily = FindQueueFamily( headless, m_handle, surface, VK_QUEUE_TRANSFER_BIT );

    uint32_t extensionCount;
    VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( m_handle, nullptr, &extensionCount, nullptr ) );
    std::vector<VkExtensionProperties> availableExtensions( extensionCount );
    VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( m_handle, nullptr, &extensionCount, availableExtensions.data() ) );
    m_availableExtensions.resize( availableExtensions.size() );
    for ( size_t ext = 0; ext < availableExtensions.size(); ++ext )
    {
        m_availableExtensions[ext] = availableExtensions[ext].extensionName;
    }

    m_rating = RateDevice( headless, surface );
}

bool PhysicalDevice::ExtensionSupported( const std::string& extensionName ) const
{
    return std::find( m_availableExtensions.begin(), m_availableExtensions.end(), extensionName ) != m_availableExtensions.end();
}

int PhysicalDevice::RateDevice( bool headless, VkSurfaceKHR surface, bool logIfUnsuitable ) const
{
    if ( m_GCTQueueFamily == ~0u )
    {
        if ( logIfUnsuitable )
            LOG_ERR( "\t\tNo combined queue available" );
        return false;
    }

    std::vector<const char*> requiredExtensions;
    for ( int i = 0; i < ARRAY_COUNT( REQUIRED_VK_EXTENSIONS ) - 1; ++i )
    {
        requiredExtensions.push_back( REQUIRED_VK_EXTENSIONS[i] );
    }
    if ( !headless )
    {
        for ( int i = 0; i < ARRAY_COUNT( REQUIRED_VK_NON_HEADLESS_EXTENSIONS ) - 1; ++i )
        {
            requiredExtensions.push_back( REQUIRED_VK_NON_HEADLESS_EXTENSIONS[i] );
        }
    }

    for ( const auto& extension : requiredExtensions )
    {
        if ( !ExtensionSupported( extension ) )
        {
            if ( logIfUnsuitable )
                LOG_ERR( "\t\tDoes not support required extension '%s'", extension );
            return 0;
        }
    }

    if ( !headless && !HasSwapChainSupport( m_handle, surface ) )
    {
        if ( logIfUnsuitable )
            LOG_ERR( "\t\tNo swapchain support" );
        return 0;
    }

    if ( !m_features.anisotropy || !m_features.bindless || !m_features.nullDescriptors
#if USING( PG_RTX )
         || !m_features.raytracing
#endif // USING( PG_RTX )
    )
    {
        if ( logIfUnsuitable )
        {
            if ( !m_features.anisotropy )
                LOG_ERR( "\t\tNo support for anisotropy" );
            if ( !m_features.bindless )
                LOG_ERR( "\t\tNo support for bindless textures" );
            if ( !m_features.nullDescriptors )
                LOG_ERR( "N\t\to support for nullDescriptors" );
            if ( !m_features.raytracing )
                LOG_ERR( "\t\tNo support for raytracing" );
        }
        return 0;
    }

    int score = 10;
    if ( m_properties.isDiscrete )
    {
        score += 1000;
    }

    return score;
}

std::string PhysicalDevice::GetName() const { return m_properties.name; }
VkPhysicalDevice PhysicalDevice::GetHandle() const { return m_handle; }
PhysicalDeviceProperties PhysicalDevice::GetProperties() const { return m_properties; }
PhysicalDeviceFeatures PhysicalDevice::GetFeatures() const { return m_features; }
VkPhysicalDeviceMemoryProperties PhysicalDevice::GetMemoryProperties() const { return m_memProperties; }
uint32_t PhysicalDevice::GetGCTQueueFamily() const { return m_GCTQueueFamily; }
uint32_t PhysicalDevice::GetGraphicsQueueFamily() const { return m_graphicsFamily; }
uint32_t PhysicalDevice::GetComputeQueueFamily() const { return m_computeFamily; }
uint32_t PhysicalDevice::GetTransferQueueFamily() const { return m_transferFamily; }
int PhysicalDevice::GetRating() const { return m_rating; }

std::vector<PhysicalDevice> EnumerateCompatiblePhysicalDevices( VkInstance instance, VkSurfaceKHR surface, bool headless, bool verbose )
{
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr ) );
    if ( deviceCount == 0 )
    {
        LOG_ERR( "No Vulkan physical devices found!" );
        return {};
    }

    std::vector<VkPhysicalDevice> vkDevices( deviceCount );
    VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &deviceCount, vkDevices.data() ) );

    std::vector<PhysicalDevice> devices;
    devices.reserve( deviceCount );
    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        PhysicalDevice dev( vkDevices[i], surface, headless );
        if ( dev.GetRating() > 0 )
        {
            devices.push_back( dev );
        }

        if ( verbose )
        {
            const auto& properties = dev.GetProperties();
            LOG( "Device %u: '%s', api = %u.%u.%u. Rating: %d", i, dev.GetProperties().name.c_str(), properties.apiVersionMajor,
                properties.apiVersionMinor, properties.apiVersionPatch, dev.GetRating() );
        }
    }

    // If none are suitable, go back and display why they arent suitable
    if ( devices.empty() )
    {
        LOG_ERR( "No Vulkan physical device was suitable! Reasons why:" );
        for ( uint32_t i = 0; i < deviceCount; ++i )
        {
            PhysicalDevice dev( vkDevices[i], surface, headless );
            const auto& properties = dev.GetProperties();
            LOG_ERR( "\tDevice %u: '%s', api = %u.%u.%u. Rating: %d", i, dev.GetProperties().name.c_str(), properties.apiVersionMajor,
                properties.apiVersionMinor, properties.apiVersionPatch, dev.GetRating() );
            dev.RateDevice( headless, surface, true );
        }
    }

    return devices;
}

PhysicalDevice SelectBestPhysicalDevice( std::vector<PhysicalDevice>& devices, int deviceOverride )
{
    PG_ASSERT( devices.size() > 0 );
    if ( deviceOverride != -1 )
    {
        PG_ASSERT( deviceOverride < (int)devices.size() );
        return devices[deviceOverride];
    }

    std::sort( devices.begin(), devices.end(),
        []( const PhysicalDevice& lhs, const PhysicalDevice& rhs ) { return lhs.GetRating() > rhs.GetRating(); } );
    return devices[0];
}

} // namespace Gfx
} // namespace PG
