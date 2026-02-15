#include "physical_device.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "shared/logger.hpp"

namespace PG::Gfx
{

static void GetDeviceProperties( VkPhysicalDevice pDev, PhysicalDeviceMetadata& metadata )
{
    void* chain = nullptr;
#if 1 || USING( PG_DESCRIPTOR_BUFFER )
    metadata.dbProps       = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
    metadata.dbProps.pNext = chain;
    chain                  = &metadata.dbProps;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

    VkPhysicalDeviceProperties2 vkProps2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    vkProps2.pNext                       = chain;
    vkGetPhysicalDeviceProperties2( pDev, &vkProps2 );
    memcpy( &metadata.properties, &vkProps2.properties, sizeof( VkPhysicalDeviceProperties ) );

    vkGetPhysicalDeviceMemoryProperties( pDev, &metadata.memProperties );
}

static void QueueSelection( VkPhysicalDevice pDev, PhysicalDeviceMetadata& metadata )
{
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( pDev, &queueFamilyCount, nullptr );
    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( pDev, &queueFamilyCount, queueFamilies.data() );
    u32 queueFamily{ 0 };
    for ( u32 i = 0; i < queueFamilyCount; ++i )
    {
        bool graphics   = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        bool compute    = queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
        bool transfer   = queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
        bool timestamps = queueFamilies[i].timestampValidBits > 0;
        VkBool32 present;
        vkGetPhysicalDeviceSurfaceSupportKHR( pDev, i, rg.surface, &present );
        // LOG( "Queue family[%u]: graphics: %d, compute: %d, transfer: %d, present: %d, timestamps: %d", i, graphics, compute, transfer,
        //     (bool)present, timestamps );
        // LOG( "    Flags: %u, QueueCount: %u", queueFamilies[i].queueFlags, queueFamilies[i].queueCount );
        if ( queueFamilies[i].queueCount == 0 )
            continue;

        if ( metadata.mainQueueFamilyIndex == INVALID_QUEUE_FAMILY )
        {
            if ( graphics && compute && transfer && present && timestamps )
                metadata.mainQueueFamilyIndex = i;
        }
        if ( metadata.transferQueueFamilyIndex == INVALID_QUEUE_FAMILY )
        {
            if ( transfer && !graphics && !compute && !present )
                metadata.transferQueueFamilyIndex = i;
        }
    }
}

bool PhysicalDevice::Init( VkPhysicalDevice pDev )
{
    m_metadata = std::make_shared<PhysicalDeviceMetadata>();
    m_handle   = pDev;
    GetDeviceProperties( m_handle, *m_metadata );

    QueueSelection( m_handle, *m_metadata );

    if ( !m_metadata->extensions.QuerySupport( pDev ) )
        return false;

    PhysicalDeviceFeatures supportedFeatures = {};
    supportedFeatures.Initialize( m_metadata->extensions );
    vkGetPhysicalDeviceFeatures2( pDev, &supportedFeatures.features2 );

    m_metadata->features = {};
    m_metadata->features.Initialize( m_metadata->extensions );
    FillFeaturesToEnableStruct( supportedFeatures, m_metadata->features );
    if ( !m_metadata->features.CheckSuitability() )
        return false;

    CalculateSuitabilityScore();
    return true;
}

void PhysicalDevice::LogReasonsForInsuitability() const
{
    LOG( "Physical Device '%s' is insuitable. Reasons:", m_metadata->properties.deviceName );
    if ( m_metadata->mainQueueFamilyIndex == INVALID_QUEUE_FAMILY )
    {
        LOG( "No queue family found that supports graphics, compute, transfer, present, and timestamps" );
        return;
    }
    if ( !m_metadata->extensions.allRequiredExtensionsPresent )
    {
        LOG( "  Device is missing the following required extensions:" );
        m_metadata->extensions.LogMissingExtensions();
        return;
    }

    LOG( "  Device is missing the following required features:" );
    m_metadata->features.LogMissingFeatures();
}

void PhysicalDevice::CalculateSuitabilityScore()
{
    // only called internally if device has all required extensions + features
    m_metadata->suitabilityScore = 10;
    if ( m_metadata->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        m_metadata->suitabilityScore += 1000;
    if ( m_metadata->transferQueueFamilyIndex != INVALID_QUEUE_FAMILY )
        m_metadata->suitabilityScore += 100;
}

std::string PhysicalDevice::GetName() const { return m_metadata->properties.deviceName; }
VkPhysicalDevice PhysicalDevice::GetHandle() const { return m_handle; }
PhysicalDevice::operator bool() const { return m_handle != VK_NULL_HANDLE; }
PhysicalDevice::operator VkPhysicalDevice() const { return m_handle; }
const PhysicalDeviceMetadata* PhysicalDevice::GetMetadata() const { return m_metadata.get(); }

} // namespace PG::Gfx
