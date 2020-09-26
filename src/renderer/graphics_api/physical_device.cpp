#include "physical_device.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include <algorithm>

extern std::vector< const char* > VK_DEVICE_EXTENSIONS;

#define INVALID_QUEUE_FAMILY ~0u


/** \brief Find and select the first avaiable queues for graphics and presentation
* Queues are where commands get submitted to and are processed asynchronously. Some queues
* might only be usable for certain operations, like graphics or memory operations.
*/
static void FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& graphicsFamily, uint32_t& presentationFamily, uint32_t& computeFamily )
{
    graphicsFamily     = INVALID_QUEUE_FAMILY;
    presentationFamily = INVALID_QUEUE_FAMILY;
    computeFamily      = INVALID_QUEUE_FAMILY;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
    {
        if ( graphicsFamily != INVALID_QUEUE_FAMILY && presentationFamily != INVALID_QUEUE_FAMILY && computeFamily != INVALID_QUEUE_FAMILY )
        {
            break;
        }

        if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if ( queueFamilies[i].queueCount > 0 && presentSupport )
            {
                presentationFamily = i;
                graphicsFamily = i;
            }
        }

        // check for dedicated compute queue
        if ( queueFamilies[i].queueCount > 0 && ( queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) && ( ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) == 0 ) )
        {
            computeFamily = i;
        }
    }

    // if there existed no queuefamily for both graphics and presenting, then look for separate ones
    if ( graphicsFamily == ~0u )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
        {
            if ( graphicsFamily != INVALID_QUEUE_FAMILY && presentationFamily != INVALID_QUEUE_FAMILY && computeFamily != INVALID_QUEUE_FAMILY )
            {
                break;
            }

            // check if the queue supports graphics operations
            if ( queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                graphicsFamily = i;
            }

            // check if the queue supports presenting images to the surface
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

            if ( queueFamilies[i].queueCount > 0 && presentSupport )
            {
                presentationFamily = i;
            }
        }
    }

    // Look for first compute queue if no dedicated one exists
    if ( graphicsFamily == INVALID_QUEUE_FAMILY || presentationFamily == INVALID_QUEUE_FAMILY || computeFamily == INVALID_QUEUE_FAMILY )
    {
        for ( uint32_t i = 0; i < static_cast< uint32_t >( queueFamilies.size() ); ++i )
        {
            // check if the queue supports graphics operations
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

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 )
    {
        formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 )
    {
        presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, presentModes.data() );
    }

    return !formats.empty() && !presentModes.empty();
}


namespace PG
{
namespace Gfx
{


/** Return a rating of how good this device is. 0 is incompatible, and the higher the better. */
static int RatePhysicalDevice( const PhysicalDevice& dev )
{
    if ( dev.GetGraphicsQueueFamily() == INVALID_QUEUE_FAMILY || dev.GetPresentationQueueFamily() == INVALID_QUEUE_FAMILY || dev.GetComputeQueueFamily() == INVALID_QUEUE_FAMILY )
    {
        return 0;
    }

    for ( const auto& extension : VK_DEVICE_EXTENSIONS )
    {
        if ( !dev.ExtensionSupported( extension ) )
        {
            return 0;
        }
    }

    if ( !HasSwapChainSupport( dev.GetHandle(), r_globals.surface ) )
    {
        return 0;
    }

    if ( !dev.GetFeatures().samplerAnisotropy )
    {
        return 0;
    }

    int score = 10;
    if ( dev.GetProperties().deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    {
        score += 1000;
    }
    
    return score;
}


bool PhysicalDevice::Select( std::string preferredGpu )
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( r_globals.instance, &deviceCount, nullptr );

    if ( deviceCount == 0 )
    {
        return false;
    }

    std::vector< VkPhysicalDevice > vkDevices( deviceCount );
    vkEnumeratePhysicalDevices( r_globals.instance, &deviceCount, vkDevices.data() );

    std::vector< PhysicalDevice > devices( deviceCount );
    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        devices[i].m_handle  = vkDevices[i];
        vkGetPhysicalDeviceProperties( vkDevices[i], &devices[i].m_deviceProperties );
        vkGetPhysicalDeviceFeatures( vkDevices[i], &devices[i].m_deviceFeatures );
        devices[i].m_deviceFeatures.samplerAnisotropy = VK_TRUE;

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( devices[i].m_handle, nullptr, &extensionCount, nullptr );
        std::vector< VkExtensionProperties > availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( devices[i].m_handle, nullptr, &extensionCount, availableExtensions.data() );
        devices[i].m_availableExtensions.resize( availableExtensions.size() );
        for ( size_t ext = 0; ext < availableExtensions.size(); ++ext )
        {
            devices[i].m_availableExtensions[ext] = availableExtensions[ext].extensionName;
        }

        FindQueueFamilies( vkDevices[i], r_globals.surface, devices[i].m_graphicsFamily, devices[i].m_presentFamily, devices[i].m_computeFamily );
        devices[i].m_name  = devices[i].m_deviceProperties.deviceName;
        devices[i].score   = RatePhysicalDevice( devices[i] );
    }
    
    bool gpuFound = false;
    if ( preferredGpu != "" )
    {
        for ( const auto& device : devices )
        {
            if ( device.m_name == preferredGpu )
            {
                *this = device;
                break;
            }
        }

        if ( r_globals.physicalDevice.m_name != preferredGpu )
        {
            LOG_WARN( "GPU '%s' not found. Checking other gpus\n" );
        }
        else if ( r_globals.physicalDevice.score <= 0 )
        {
            LOG_WARN( "GPU '%s' is not a compatible device for this engine. Checking other gpus\n" );
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


std::string PhysicalDevice::GetName() const                                  { return m_name; }
VkPhysicalDevice PhysicalDevice::GetHandle() const                           { return m_handle; }
VkPhysicalDeviceProperties PhysicalDevice::GetProperties() const             { return m_deviceProperties; }
VkPhysicalDeviceFeatures PhysicalDevice::GetFeatures() const                 { return m_deviceFeatures; }
VkPhysicalDeviceMemoryProperties PhysicalDevice::GetMemoryProperties() const { return m_memProperties; }
uint32_t PhysicalDevice::GetGraphicsQueueFamily() const                      { return m_graphicsFamily; }
uint32_t PhysicalDevice::GetPresentationQueueFamily() const                  { return m_presentFamily; }
uint32_t PhysicalDevice::GetComputeQueueFamily() const                       { return m_computeFamily; }


} // namespace Gfx
} // namespace PG
