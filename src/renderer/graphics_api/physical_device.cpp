#include "physical_device.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include <algorithm>

#define INVALID_QUEUE_FAMILY ~0u


/** \brief Find and select the first avaiable queues for graphics and presentation
* Queues are where commands get submitted to and are processed asynchronously. Some queues
* might only be usable for certain operations, like graphics or memory operations.
*/
static void FindQueueFamilies( bool headless, VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& graphicsFamily, uint32_t& presentationFamily, uint32_t& computeFamily )
{
    graphicsFamily     = INVALID_QUEUE_FAMILY;
    presentationFamily = INVALID_QUEUE_FAMILY;
    computeFamily      = INVALID_QUEUE_FAMILY;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );
    std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    auto AllQueuesComplete = [&]() { return graphicsFamily != INVALID_QUEUE_FAMILY && presentationFamily != INVALID_QUEUE_FAMILY && computeFamily != INVALID_QUEUE_FAMILY; };
    for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
    {
        if ( AllQueuesComplete() )
        {
            break;
        }

        if ( queueFamilies[i].queueCount > 0 )
        {
            if ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                if ( headless )
                {
                    graphicsFamily = i;
                }
                else
                {
                    VkBool32 presentSupport = false;
                    VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport ) );

                    if ( queueFamilies[i].queueCount > 0 && presentSupport )
                    {
                        presentationFamily = i;
                        graphicsFamily = i;
                    }
                }
            }
            // check for dedicated compute queue, separate from the graphics queue
            else if ( queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                computeFamily = i;
            }
        }
    }

    // if there existed no queuefamily for both graphics and presenting, then look for separate ones
    if ( !headless && graphicsFamily == ~0u )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
        {
            if ( AllQueuesComplete() )
            {
                break;
            }

            if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            VK_CHECK_RESULT( vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport ) );

            if ( queueFamilies[i].queueCount > 0 && presentSupport )
            {
                presentationFamily = i;
            }
        }
    }

    // If no dedicated compute queue exists, grab 
    if ( computeFamily == INVALID_QUEUE_FAMILY )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
        {
            if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
            {
                computeFamily = i;
                break;
            }
        }
    }
}


static bool HasSwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector< VkSurfaceFormatKHR > formats;
    std::vector< VkPresentModeKHR > presentModes;

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


// Return a rating of how good this device is. 0 is incompatible, and the higher the better
static int RatePhysicalDevice( const PhysicalDevice& dev, bool headless )
{
    if ( dev.GetGraphicsQueueFamily() == INVALID_QUEUE_FAMILY || dev.GetComputeQueueFamily() == INVALID_QUEUE_FAMILY )
    {
        return 0;
    }

    if ( !headless && dev.GetPresentationQueueFamily() == INVALID_QUEUE_FAMILY )
    {
        return 0;
    }

    std::vector< const char* > extensions;
    if ( !headless )
    {
        extensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    }

    for ( const auto& extension : extensions )
    {
        if ( !dev.ExtensionSupported( extension ) )
        {
            return 0;
        }
    }

    if ( !headless && !HasSwapChainSupport( dev.GetHandle(), r_globals.surface ) )
    {
        return 0;
    }

    const auto& features = dev.GetFeatures();
    if ( !features.anisotropy || !features.bindless )
    {
        return 0;
    }

    int score = 10;
    if ( dev.GetProperties().isDiscrete )
    {
        score += 1000;
    }
    
    return score;
}


static PhysicalDeviceProperties GetDeviceProperties( VkPhysicalDevice physicalDevice )
{
    VkPhysicalDeviceProperties vkProperties;
    vkGetPhysicalDeviceProperties( physicalDevice, &vkProperties );
    PhysicalDeviceProperties p = {};
    p.name            = vkProperties.deviceName;
    p.apiVersionMajor = VK_VERSION_MAJOR( vkProperties.apiVersion );
    p.apiVersionMinor = VK_VERSION_MINOR( vkProperties.apiVersion );
    p.apiVersionPatch = VK_VERSION_PATCH( vkProperties.apiVersion );
    p.isDiscrete      = vkProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    // Vulkan's timestampPeriod is expressed in nanoseconds per clock tick
    p.timestampFrequency = 1e9 / vkProperties.limits.timestampPeriod;

    return p;
}


static PhysicalDeviceFeatures GetDeviceFeatures( VkPhysicalDevice physicalDevice )
{
    VkPhysicalDeviceFeatures vkFeatures;
    vkGetPhysicalDeviceFeatures( physicalDevice, &vkFeatures );
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures{};
    indexingFeatures.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexingFeatures.pNext = nullptr;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &indexingFeatures;
    vkGetPhysicalDeviceFeatures2( physicalDevice, &deviceFeatures );
    
    PhysicalDeviceFeatures f = {};
    f.anisotropy = vkFeatures.samplerAnisotropy == VK_TRUE;
    if ( indexingFeatures.descriptorBindingPartiallyBound && indexingFeatures.runtimeDescriptorArray )
    {
	    f.bindless = true;
    }

    return f;
}


bool PhysicalDevice::Select( bool headless, std::string preferredGpu )
{
    uint32_t deviceCount = 0;
    VK_CHECK_RESULT( vkEnumeratePhysicalDevices( r_globals.instance, &deviceCount, nullptr ) );

    if ( deviceCount == 0 )
    {
        return false;
    }

    std::vector< VkPhysicalDevice > vkDevices( deviceCount );
    VK_CHECK_RESULT( vkEnumeratePhysicalDevices( r_globals.instance, &deviceCount, vkDevices.data() ) );

    std::vector< PhysicalDevice > devices( deviceCount );
    bool runningInRenderDoc = false;
    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        devices[i].m_handle  = vkDevices[i];
        devices[i].m_Properties = GetDeviceProperties( vkDevices[i] );
        devices[i].m_Features   = GetDeviceFeatures( vkDevices[i] );

        uint32_t extensionCount;
        VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( devices[i].m_handle, nullptr, &extensionCount, nullptr ) );
        std::vector< VkExtensionProperties > availableExtensions( extensionCount );
        VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( devices[i].m_handle, nullptr, &extensionCount, availableExtensions.data() ) );
        devices[i].m_availableExtensions.resize( availableExtensions.size() );
        for ( size_t ext = 0; ext < availableExtensions.size(); ++ext )
        {
            devices[i].m_availableExtensions[ext] = availableExtensions[ext].extensionName;
        }

        FindQueueFamilies( headless, vkDevices[i], r_globals.surface, devices[i].m_graphicsFamily, devices[i].m_presentFamily, devices[i].m_computeFamily );
        devices[i].score = RatePhysicalDevice( devices[i], headless );
    }

    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        auto p = devices[i].GetProperties();
        LOG( "Device %u: '%s', api = %u.%u.%u", i, p.name.c_str(), p.apiVersionMajor, p.apiVersionMinor, p.apiVersionPatch );
    }
    
    bool gpuFound = false;
    if ( preferredGpu != "" )
    {
        for ( const auto& device : devices )
        {
            if ( device.GetName() == preferredGpu )
            {
                *this = device;
                break;
            }
        }

        if ( r_globals.physicalDevice.GetName() != preferredGpu )
        {
            LOG_WARN( "GPU '%s' not found. Checking other gpus", preferredGpu.c_str() );
        }
        else if ( r_globals.physicalDevice.score <= 0 )
        {
            LOG_WARN( "GPU '%s' is not a compatible device for this engine. Checking other gpus", preferredGpu.c_str() );
        }
        else
        {
            gpuFound = true;
        }
    }

    if ( !gpuFound )
    {
        std::sort( devices.begin(), devices.end(), []( const auto& lhs, const auto& rhs ) { return lhs.score > rhs.score; } );
        *this = devices[0];
    }

    if ( r_globals.physicalDevice.score <= 0 )
    {
        m_handle = VK_NULL_HANDLE;
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties( m_handle, &m_memProperties );
    
    return true;
}


bool PhysicalDevice::ExtensionSupported( const std::string& extensionName ) const
{
    return std::find( m_availableExtensions.begin(), m_availableExtensions.end(), extensionName ) != m_availableExtensions.end();
}


std::string PhysicalDevice::GetName() const                                  { return m_Properties.name; }
VkPhysicalDevice PhysicalDevice::GetHandle() const                           { return m_handle; }
PhysicalDeviceProperties PhysicalDevice::GetProperties() const               { return m_Properties; }
PhysicalDeviceFeatures PhysicalDevice::GetFeatures() const                   { return m_Features; }
VkPhysicalDeviceMemoryProperties PhysicalDevice::GetMemoryProperties() const { return m_memProperties; }
uint32_t PhysicalDevice::GetGraphicsQueueFamily() const                      { return m_graphicsFamily; }
uint32_t PhysicalDevice::GetPresentationQueueFamily() const                  { return m_presentFamily; }
uint32_t PhysicalDevice::GetComputeQueueFamily() const                       { return m_computeFamily; }


} // namespace Gfx
} // namespace PG
