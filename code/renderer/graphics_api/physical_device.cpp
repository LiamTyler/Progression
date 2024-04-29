#include "physical_device.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "shared/logger.hpp"

namespace PG::Gfx
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

    p.dbProps = {};
    p.dbProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;

    VkPhysicalDeviceProperties2 vkProperties2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    vkProperties2.pNext = &p.dbProps;
    vkGetPhysicalDeviceProperties2( physicalDevice, &vkProperties2 );

    return p;
}

void PhysicalDevice::Create( VkPhysicalDevice pDev )
{
    m_handle     = pDev;
    m_properties = GetDeviceProperties( m_handle );
    vkGetPhysicalDeviceMemoryProperties( m_handle, &m_memProperties );
}

std::string PhysicalDevice::GetName() const { return m_properties.name; }
VkPhysicalDevice PhysicalDevice::GetHandle() const { return m_handle; }
const PhysicalDeviceProperties& PhysicalDevice::GetProperties() const { return m_properties; }
VkPhysicalDeviceMemoryProperties PhysicalDevice::GetMemoryProperties() const { return m_memProperties; }

} // namespace PG::Gfx
