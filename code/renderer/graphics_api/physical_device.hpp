#pragma once

#include "renderer/vulkan.hpp"
#include <string>

namespace PG::Gfx
{

struct PhysicalDeviceProperties
{
    std::string name;
    f64 nanosecondsPerTick; // # timestamps per second
    u16 apiVersionMajor;
    u16 apiVersionMinor;
    u16 apiVersionPatch;
    bool isDiscrete;
    VkPhysicalDeviceDescriptorBufferPropertiesEXT dbProps;
    f32 maxAnisotropy;
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
