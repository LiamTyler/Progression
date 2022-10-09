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
    bool raytracing = false;
};

class PhysicalDevice
{
public:
    PhysicalDevice() = default;
    PhysicalDevice( VkPhysicalDevice vkPhysicalDevice, VkSurfaceKHR surface, bool headless );

    bool ExtensionSupported( const std::string& extensionName ) const;
    // compatible devices have a strictly positive score. The higher, the better
    int RateDevice( bool headless, VkSurfaceKHR surface ) const;

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    PhysicalDeviceProperties GetProperties() const;
    PhysicalDeviceFeatures GetFeatures() const;
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;
    uint32_t GetGCTQueueFamily() const;
    uint32_t GetGraphicsQueueFamily() const;
    uint32_t GetComputeQueueFamily() const;
    uint32_t GetTransferQueueFamily() const;
    int GetRating() const;

private:
    VkPhysicalDevice m_handle;
    PhysicalDeviceProperties m_properties;
    PhysicalDeviceFeatures m_features;
    VkPhysicalDeviceMemoryProperties m_memProperties;
    uint32_t m_GCTQueueFamily; // graphics, compute, and transfer (and present, if not headless)
    uint32_t m_graphicsFamily;
    uint32_t m_computeFamily;    
    uint32_t m_transferFamily;    
    std::vector<std::string> m_availableExtensions;
    int m_rating; // only used in SelectBestPhysicalDevice()
};

std::vector<PhysicalDevice> EnumerateCompatiblePhysicalDevices( VkInstance instance, VkSurfaceKHR surface, bool headless, bool verbose );
PhysicalDevice SelectBestPhysicalDevice( std::vector<PhysicalDevice>& devices, int deviceOverride = -1 );

} // namespace Gfx
} // namespace PG
