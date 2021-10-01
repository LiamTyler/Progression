#pragma once

#include "renderer/vulkan.hpp"
#include <string>
#include <vector>

namespace PG
{
namespace Gfx
{


struct PhysicalDeviceProperties
{
    std::string name;
    double timestampFrequency; // # timestamps per second
    uint16_t apiVersionMajor;
    uint16_t apiVersionMinor;
    uint16_t apiVersionPatch;
    bool isDiscrete;
};

struct PhysicalDeviceFeatures
{
    bool anisotropy = false;
    bool bindless = false;
    bool nullDescriptors = false;
};

class PhysicalDevice
{
public:

    bool Select( bool headless, std::string preferredGpu = "" );
    bool ExtensionSupported( const std::string& extensionName ) const;

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    PhysicalDeviceProperties GetProperties() const;
    PhysicalDeviceFeatures GetFeatures() const;
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;
    uint32_t GetGraphicsQueueFamily() const;
    uint32_t GetPresentationQueueFamily() const;
    uint32_t GetComputeQueueFamily() const;

private:
    VkPhysicalDevice m_handle;
    PhysicalDeviceProperties m_Properties;
    PhysicalDeviceFeatures m_Features;
    VkPhysicalDeviceMemoryProperties m_memProperties;
    uint32_t m_graphicsFamily;
    uint32_t m_presentFamily;
    uint32_t m_computeFamily;    
    std::vector< std::string > m_availableExtensions;
    int score; // only used during device selection
};

} // namespace Gfx
} // namespace PG
