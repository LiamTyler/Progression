#include "renderer/vulkan.hpp"
#include "core/feature_defines.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"

#ifdef VK_KHR_acceleration_structure

static PFN_vkBuildAccelerationStructuresKHR pfn_vkBuildAccelerationStructuresKHR                                 = VK_NULL_HANDLE;
static PFN_vkCmdBuildAccelerationStructuresIndirectKHR pfn_vkCmdBuildAccelerationStructuresIndirectKHR           = VK_NULL_HANDLE;
static PFN_vkCmdBuildAccelerationStructuresKHR pfn_vkCmdBuildAccelerationStructuresKHR                           = VK_NULL_HANDLE;
static PFN_vkCmdCopyAccelerationStructureKHR pfn_vkCmdCopyAccelerationStructureKHR                               = VK_NULL_HANDLE;
static PFN_vkCmdCopyAccelerationStructureToMemoryKHR pfn_vkCmdCopyAccelerationStructureToMemoryKHR               = VK_NULL_HANDLE;
static PFN_vkCmdCopyMemoryToAccelerationStructureKHR pfn_vkCmdCopyMemoryToAccelerationStructureKHR               = VK_NULL_HANDLE;
static PFN_vkCmdWriteAccelerationStructuresPropertiesKHR pfn_vkCmdWriteAccelerationStructuresPropertiesKHR       = VK_NULL_HANDLE;
static PFN_vkCopyAccelerationStructureKHR pfn_vkCopyAccelerationStructureKHR                                     = VK_NULL_HANDLE;
static PFN_vkCopyAccelerationStructureToMemoryKHR pfn_vkCopyAccelerationStructureToMemoryKHR                     = VK_NULL_HANDLE;
static PFN_vkCopyMemoryToAccelerationStructureKHR pfn_vkCopyMemoryToAccelerationStructureKHR                     = VK_NULL_HANDLE;
static PFN_vkCreateAccelerationStructureKHR pfn_vkCreateAccelerationStructureKHR                                 = VK_NULL_HANDLE;
static PFN_vkDestroyAccelerationStructureKHR pfn_vkDestroyAccelerationStructureKHR                               = VK_NULL_HANDLE;
static PFN_vkGetAccelerationStructureBuildSizesKHR pfn_vkGetAccelerationStructureBuildSizesKHR                   = VK_NULL_HANDLE;
static PFN_vkGetAccelerationStructureDeviceAddressKHR pfn_vkGetAccelerationStructureDeviceAddressKHR             = VK_NULL_HANDLE;
static PFN_vkGetDeviceAccelerationStructureCompatibilityKHR pfn_vkGetDeviceAccelerationStructureCompatibilityKHR = VK_NULL_HANDLE;
static PFN_vkWriteAccelerationStructuresPropertiesKHR pfn_vkWriteAccelerationStructuresPropertiesKHR             = VK_NULL_HANDLE;

VKAPI_ATTR VkResult VKAPI_CALL vkBuildAccelerationStructuresKHR( VkDevice device, VkDeferredOperationKHR deferredOperation,
    uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos )
{
    return pfn_vkBuildAccelerationStructuresKHR( device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresIndirectKHR( VkCommandBuffer commandBuffer, uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkDeviceAddress* pIndirectDeviceAddresses,
    const uint32_t* pIndirectStrides, const uint32_t* const* ppMaxPrimitiveCounts )
{
    pfn_vkCmdBuildAccelerationStructuresIndirectKHR(
        commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR( VkCommandBuffer commandBuffer, uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos )
{
    pfn_vkCmdBuildAccelerationStructuresKHR( commandBuffer, infoCount, pInfos, ppBuildRangeInfos );
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureKHR(
    VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR* pInfo )
{
    pfn_vkCmdCopyAccelerationStructureKHR( commandBuffer, pInfo );
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureToMemoryKHR(
    VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo )
{
    pfn_vkCmdCopyAccelerationStructureToMemoryKHR( commandBuffer, pInfo );
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToAccelerationStructureKHR(
    VkCommandBuffer commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo )
{
    pfn_vkCmdCopyMemoryToAccelerationStructureKHR( commandBuffer, pInfo );
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesKHR( VkCommandBuffer commandBuffer,
    uint32_t accelerationStructureCount, const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType,
    VkQueryPool queryPool, uint32_t firstQuery )
{
    pfn_vkCmdWriteAccelerationStructuresPropertiesKHR(
        commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureKHR(
    VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureInfoKHR* pInfo )
{
    return pfn_vkCopyAccelerationStructureKHR( device, deferredOperation, pInfo );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureToMemoryKHR(
    VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR* pInfo )
{
    return pfn_vkCopyAccelerationStructureToMemoryKHR( device, deferredOperation, pInfo );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToAccelerationStructureKHR(
    VkDevice device, VkDeferredOperationKHR deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR* pInfo )
{
    return pfn_vkCopyMemoryToAccelerationStructureKHR( device, deferredOperation, pInfo );
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR( VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure )
{
    return pfn_vkCreateAccelerationStructureKHR( device, pCreateInfo, pAllocator, pAccelerationStructure );
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR(
    VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks* pAllocator )
{
    pfn_vkDestroyAccelerationStructureKHR( device, accelerationStructure, pAllocator );
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR( VkDevice device, VkAccelerationStructureBuildTypeKHR buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const uint32_t* pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo )
{
    pfn_vkGetAccelerationStructureBuildSizesKHR( device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo );
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR(
    VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo )
{
    return pfn_vkGetAccelerationStructureDeviceAddressKHR( device, pInfo );
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceAccelerationStructureCompatibilityKHR(
    VkDevice device, const VkAccelerationStructureVersionInfoKHR* pVersionInfo, VkAccelerationStructureCompatibilityKHR* pCompatibility )
{
    pfn_vkGetDeviceAccelerationStructureCompatibilityKHR( device, pVersionInfo, pCompatibility );
}

VKAPI_ATTR VkResult VKAPI_CALL vkWriteAccelerationStructuresPropertiesKHR( VkDevice device, uint32_t accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType, size_t dataSize, void* pData, size_t stride )
{
    return pfn_vkWriteAccelerationStructuresPropertiesKHR(
        device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride );
}
#endif // #ifdef VK_KHR_acceleration_structure

namespace PG::Gfx
{

#if USING( DEBUG_BUILD )
    #define LOAD_VK_FUNC( funcName ) pfn_##funcName = (PFN_##funcName)vkGetDeviceProcAddr( device, #funcName );
#else // #if !USING( DEBUG_BUILD )
    #define LOAD_VK_FUNC( funcName )                                                                            \
        pfn_##funcName = (PFN_##funcName)vkGetDeviceProcAddr( device, #funcName );                              \
        if ( !pfn_##funcName )                                                                                  \
        {                                                                                                       \
            LOG_ERR( "Failed to load vulkan function '" #funcName "'. Did you forget to load the extension?" ); \
        }
#endif // #else // #if !USING( DEBUG_BUILD )

void LoadVulkanExtensions( VkDevice device )
{
#if USING( PG_RTX )
    #ifdef VK_KHR_acceleration_structure
    LOAD_VK_FUNC( vkBuildAccelerationStructuresKHR );
    LOAD_VK_FUNC( vkCmdBuildAccelerationStructuresIndirectKHR );
    LOAD_VK_FUNC( vkCmdBuildAccelerationStructuresKHR );
    LOAD_VK_FUNC( vkCmdCopyAccelerationStructureKHR );
    LOAD_VK_FUNC( vkCmdCopyAccelerationStructureToMemoryKHR );
    LOAD_VK_FUNC( vkCmdCopyMemoryToAccelerationStructureKHR );
    LOAD_VK_FUNC( vkCmdWriteAccelerationStructuresPropertiesKHR );
    LOAD_VK_FUNC( vkCopyAccelerationStructureKHR );
    LOAD_VK_FUNC( vkCopyAccelerationStructureToMemoryKHR );
    LOAD_VK_FUNC( vkCopyMemoryToAccelerationStructureKHR );
    LOAD_VK_FUNC( vkCreateAccelerationStructureKHR );
    LOAD_VK_FUNC( vkDestroyAccelerationStructureKHR );
    LOAD_VK_FUNC( vkGetAccelerationStructureBuildSizesKHR );
    LOAD_VK_FUNC( vkGetAccelerationStructureDeviceAddressKHR );
    LOAD_VK_FUNC( vkGetDeviceAccelerationStructureCompatibilityKHR );
    LOAD_VK_FUNC( vkWriteAccelerationStructuresPropertiesKHR );
    #endif // #ifdef VK_KHR_acceleration_structure
#endif     // #if USING( PG_RTX )
}

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    auto memProperties = rg.physicalDevice.GetMemoryProperties();
    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
    {
        if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
        {
            return i;
        }
    }

    PG_ASSERT( false );

    return ~0u;
}

void TransitionImageLayoutImmediate(
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layers )
{
    // CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time TransitionImageLayout" );
    // cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    //
    // VkImageMemoryBarrier barrier            = {};
    // barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    // barrier.oldLayout                       = oldLayout;
    // barrier.newLayout                       = newLayout;
    // barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    // barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    // barrier.image                           = image;
    // barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    // barrier.subresourceRange.baseMipLevel   = 0;
    // barrier.subresourceRange.levelCount     = mipLevels;
    // barrier.subresourceRange.baseArrayLayer = 0;
    // barrier.subresourceRange.layerCount     = layers;
    // barrier.srcAccessMask                   = 0;
    // barrier.dstAccessMask                   = 0;
    //
    // PipelineStageFlags srcStage;
    // PipelineStageFlags dstStage;
    // if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    //{
    //     barrier.srcAccessMask = 0;
    //     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //     srcStage              = PipelineStageFlags::TOP_OF_PIPE_BIT;
    //     dstStage              = PipelineStageFlags::TRANSFER_BIT;
    // }
    // else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    //{
    //     barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //     barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //     srcStage              = PipelineStageFlags::TRANSFER_BIT;
    //     dstStage              = PipelineStageFlags::FRAGMENT_SHADER_BIT;
    // }
    // else
    //{
    //     PG_ASSERT( false, "The transition barriers are unknown for the given old and new layouts" );
    // }
    //
    // cmdBuf.PipelineImageBarrier( srcStage, dstStage, barrier );
    //
    // cmdBuf.EndRecording();
    // rg.device.Submit( cmdBuf );
    // rg.device.WaitForIdle();
    // cmdBuf.Free();
}

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( rg.physicalDevice.GetHandle(), format, &props );
    return ( props.optimalTilingFeatures & requestedSupport ) == requestedSupport;
}

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layers )
{
    PG_ASSERT( layers == 1 || layers == 6 );
    VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    info.image                 = image;
    info.format                = format;
    info.subresourceRange      = ImageSubresourceRange( aspectFlags, 0, mipLevels, 0, layers );
    info.viewType              = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

    VkImageView view;
    VK_CHECK( vkCreateImageView( rg.device.GetHandle(), &info, nullptr, &view ) );

    return view;
}

VkImageSubresourceRange ImageSubresourceRange(
    VkImageAspectFlags aspectFlags, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount )
{
    VkImageSubresourceRange subImage{};
    subImage.aspectMask     = aspectFlags;
    subImage.baseMipLevel   = baseMipLevel;
    subImage.levelCount     = levelCount;
    subImage.baseArrayLayer = baseArrayLayer;
    subImage.layerCount     = layerCount;

    return subImage;
}

VkSemaphoreSubmitInfo SemaphoreSubmitInfo( VkPipelineStageFlags2 stageMask, VkSemaphore semaphore )
{
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.pNext       = nullptr;
    submitInfo.semaphore   = semaphore;
    submitInfo.stageMask   = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value       = 1;

    return submitInfo;
}

} // namespace PG::Gfx
