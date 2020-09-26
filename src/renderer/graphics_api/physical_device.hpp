#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace PG
{
namespace Gfx
{

class PhysicalDevice
{
public:

    bool Select( std::string preferredGpu = "" );
    bool ExtensionSupported( const std::string& extensionName ) const;

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    VkPhysicalDeviceProperties GetProperties() const;
    VkPhysicalDeviceFeatures GetFeatures() const;
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;
    uint32_t GetGraphicsQueueFamily() const;
    uint32_t GetPresentationQueueFamily() const;
    uint32_t GetComputeQueueFamily() const;

private:
    std::string m_name;
    VkPhysicalDevice m_handle;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
    VkPhysicalDeviceMemoryProperties m_memProperties;
    uint32_t m_graphicsFamily;
    uint32_t m_presentFamily;
    uint32_t m_computeFamily;    
    std::vector< std::string > m_availableExtensions;
    int score; // only used during device selection
};

} // namespace Gfx
} // namespace PG
