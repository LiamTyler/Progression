#pragma once

#include "renderer/vulkan.hpp"
#include <string>

namespace PG::Gfx
{

struct PhysicalDeviceProperties
{
    std::string name;
    double nanosecondsPerTick; // # timestamps per second
    uint16_t apiVersionMajor;
    uint16_t apiVersionMinor;
    uint16_t apiVersionPatch;
    bool isDiscrete;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT dbProps;
};

class PhysicalDevice
{
public:
    PhysicalDevice() = default;

    // R_Init() is where the device selection actually happens
    void Create( VkPhysicalDevice pDev );

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    operator bool() const;
    operator VkPhysicalDevice() const;
    const PhysicalDeviceProperties& GetProperties() const;
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;

private:
    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    PhysicalDeviceProperties m_properties;
    VkPhysicalDeviceMemoryProperties m_memProperties;
};

} // namespace PG::Gfx
