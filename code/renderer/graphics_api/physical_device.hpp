#pragma once

#include "renderer/graphics_api/features.hpp"
#include "renderer/vulkan.hpp"
#include <memory>
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

static constexpr u32 INVALID_QUEUE_FAMILY = ~0u;

struct PhysicalDeviceMetadata
{
    u32 mainQueueFamilyIndex            = INVALID_QUEUE_FAMILY;
    u32 transferQueueFamilyIndex        = INVALID_QUEUE_FAMILY;
    int suitabilityScore                = 0;
    PhysicalDeviceExtensions extensions = {};
    PhysicalDeviceFeatures features     = {};
};

class PhysicalDevice
{
public:
    PhysicalDevice() = default;

    bool Init( VkPhysicalDevice pDev );

    void LogReasonsForInsuitability() const;

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    operator bool() const;
    operator VkPhysicalDevice() const;
    const PhysicalDeviceProperties& GetProperties() const;
    VkPhysicalDeviceMemoryProperties GetMemoryProperties() const;
    const PhysicalDeviceMetadata* GetMetadata() const;

private:
    void CalculateSuitabilityScore();

    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    PhysicalDeviceProperties m_properties;
    VkPhysicalDeviceMemoryProperties m_memProperties;
    std::shared_ptr<PhysicalDeviceMetadata> m_metadata;
};

} // namespace PG::Gfx
