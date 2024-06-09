#include "renderer/vulkan.hpp"
#include "core/feature_defines.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"

#define DECLARE_VK_FUNC( name ) static PFN_##name pfn_##name = VK_NULL_HANDLE

#ifdef VK_KHR_acceleration_structure

DECLARE_VK_FUNC( vkBuildAccelerationStructuresKHR );
DECLARE_VK_FUNC( vkCmdBuildAccelerationStructuresIndirectKHR );
DECLARE_VK_FUNC( vkCmdBuildAccelerationStructuresKHR );
DECLARE_VK_FUNC( vkCmdCopyAccelerationStructureKHR );
DECLARE_VK_FUNC( vkCmdCopyAccelerationStructureToMemoryKHR );
DECLARE_VK_FUNC( vkCmdCopyMemoryToAccelerationStructureKHR );
DECLARE_VK_FUNC( vkCmdWriteAccelerationStructuresPropertiesKHR );
DECLARE_VK_FUNC( vkCopyAccelerationStructureKHR );
DECLARE_VK_FUNC( vkCopyAccelerationStructureToMemoryKHR );
DECLARE_VK_FUNC( vkCopyMemoryToAccelerationStructureKHR );
DECLARE_VK_FUNC( vkCreateAccelerationStructureKHR );
DECLARE_VK_FUNC( vkDestroyAccelerationStructureKHR );
DECLARE_VK_FUNC( vkGetAccelerationStructureBuildSizesKHR );
DECLARE_VK_FUNC( vkGetAccelerationStructureDeviceAddressKHR );
DECLARE_VK_FUNC( vkGetDeviceAccelerationStructureCompatibilityKHR );
DECLARE_VK_FUNC( vkWriteAccelerationStructuresPropertiesKHR );

VKAPI_ATTR VkResult VKAPI_CALL vkBuildAccelerationStructuresKHR( VkDevice device, VkDeferredOperationKHR deferredOperation, u32 infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos )
{
    return pfn_vkBuildAccelerationStructuresKHR( device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresIndirectKHR( VkCommandBuffer commandBuffer, u32 infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkDeviceAddress* pIndirectDeviceAddresses, const u32* pIndirectStrides,
    const u32* const* ppMaxPrimitiveCounts )
{
    pfn_vkCmdBuildAccelerationStructuresIndirectKHR(
        commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR( VkCommandBuffer commandBuffer, u32 infoCount,
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

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesKHR( VkCommandBuffer commandBuffer, u32 accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, u32 firstQuery )
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
    const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const u32* pMaxPrimitiveCounts,
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

VKAPI_ATTR VkResult VKAPI_CALL vkWriteAccelerationStructuresPropertiesKHR( VkDevice device, u32 accelerationStructureCount,
    const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType, size_t dataSize, void* pData, size_t stride )
{
    return pfn_vkWriteAccelerationStructuresPropertiesKHR(
        device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride );
}
#endif // #ifdef VK_KHR_acceleration_structure

#if USING( PG_DESCRIPTOR_BUFFER )
#if defined( VK_EXT_descriptor_buffer )
DECLARE_VK_FUNC( vkCmdBindDescriptorBufferEmbeddedSamplersEXT );
DECLARE_VK_FUNC( vkCmdBindDescriptorBuffersEXT );
DECLARE_VK_FUNC( vkCmdSetDescriptorBufferOffsetsEXT );
DECLARE_VK_FUNC( vkGetBufferOpaqueCaptureDescriptorDataEXT );
DECLARE_VK_FUNC( vkGetDescriptorEXT );
DECLARE_VK_FUNC( vkGetDescriptorSetLayoutBindingOffsetEXT );
DECLARE_VK_FUNC( vkGetDescriptorSetLayoutSizeEXT );
DECLARE_VK_FUNC( vkGetImageOpaqueCaptureDescriptorDataEXT );
DECLARE_VK_FUNC( vkGetImageViewOpaqueCaptureDescriptorDataEXT );
DECLARE_VK_FUNC( vkGetSamplerOpaqueCaptureDescriptorDataEXT );
DECLARE_VK_FUNC( vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT );

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSizeEXT(
    VkDevice device, VkDescriptorSetLayout layout, VkDeviceSize* pLayoutSizeInBytes )
{
    pfn_vkGetDescriptorSetLayoutSizeEXT( device, layout, pLayoutSizeInBytes );
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutBindingOffsetEXT(
    VkDevice device, VkDescriptorSetLayout layout, u32 binding, VkDeviceSize* pOffset )
{
    pfn_vkGetDescriptorSetLayoutBindingOffsetEXT( device, layout, binding, pOffset );
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorEXT(
    VkDevice device, const VkDescriptorGetInfoEXT* pDescriptorInfo, size_t dataSize, void* pDescriptor )
{
    pfn_vkGetDescriptorEXT( device, pDescriptorInfo, dataSize, pDescriptor );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBuffersEXT(
    VkCommandBuffer commandBuffer, u32 bufferCount, const VkDescriptorBufferBindingInfoEXT* pBindingInfos )
{
    pfn_vkCmdBindDescriptorBuffersEXT( commandBuffer, bufferCount, pBindingInfos );
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDescriptorBufferOffsetsEXT( VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, u32 firstSet, u32 setCount, const u32* pBufferIndices, const VkDeviceSize* pOffsets )
{
    pfn_vkCmdSetDescriptorBufferOffsetsEXT( commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pBufferIndices, pOffsets );
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, u32 set )
{
    pfn_vkCmdBindDescriptorBufferEmbeddedSamplersEXT( commandBuffer, pipelineBindPoint, layout, set );
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetBufferOpaqueCaptureDescriptorDataEXT(
    VkDevice device, const VkBufferCaptureDescriptorDataInfoEXT* pInfo, void* pData )
{
    return pfn_vkGetBufferOpaqueCaptureDescriptorDataEXT( device, pInfo, pData );
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageOpaqueCaptureDescriptorDataEXT(
    VkDevice device, const VkImageCaptureDescriptorDataInfoEXT* pInfo, void* pData )
{
    return pfn_vkGetImageOpaqueCaptureDescriptorDataEXT( device, pInfo, pData );
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageViewOpaqueCaptureDescriptorDataEXT(
    VkDevice device, const VkImageViewCaptureDescriptorDataInfoEXT* pInfo, void* pData )
{
    return pfn_vkGetImageViewOpaqueCaptureDescriptorDataEXT( device, pInfo, pData );
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSamplerOpaqueCaptureDescriptorDataEXT(
    VkDevice device, const VkSamplerCaptureDescriptorDataInfoEXT* pInfo, void* pData )
{
    return pfn_vkGetSamplerOpaqueCaptureDescriptorDataEXT( device, pInfo, pData );
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
    VkDevice device, const VkAccelerationStructureCaptureDescriptorDataInfoEXT* pInfo, void* pData )
{
    return pfn_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT( device, pInfo, pData );
}
#endif // #if defined( VK_EXT_descriptor_buffer )
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

DECLARE_VK_FUNC( vkCmdDrawMeshTasksIndirectCountEXT );
DECLARE_VK_FUNC( vkCmdDrawMeshTasksIndirectEXT );
DECLARE_VK_FUNC( vkCmdDrawMeshTasksEXT );

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountEXT( VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, u32 maxDrawCount, u32 stride )
{
    pfn_vkCmdDrawMeshTasksIndirectCountEXT( commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride );
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectEXT(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, u32 drawCount, u32 stride )
{
    pfn_vkCmdDrawMeshTasksIndirectEXT( commandBuffer, buffer, offset, drawCount, stride );
}
VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksEXT( VkCommandBuffer commandBuffer, u32 groupCountX, u32 groupCountY, u32 groupCountZ )
{
    pfn_vkCmdDrawMeshTasksEXT( commandBuffer, groupCountX, groupCountY, groupCountZ );
}

namespace PG::Gfx
{

#if USING( SHIP_BUILD )
#define LOAD_VK_FUNC( funcName ) pfn_##funcName = (PFN_##funcName)vkGetDeviceProcAddr( device, #funcName );
#else // #if USING( SHIP_BUILD )
#define LOAD_VK_FUNC( funcName )                                                                            \
    pfn_##funcName = (PFN_##funcName)vkGetDeviceProcAddr( device, #funcName );                              \
    if ( !pfn_##funcName )                                                                                  \
    {                                                                                                       \
        LOG_ERR( "Failed to load vulkan function '" #funcName "'. Did you forget to load the extension?" ); \
    }
#endif // #else // #if USING( SHIP_BUILD )

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
#endif // #if USING( PG_RTX )

#if USING( PG_DESCRIPTOR_BUFFER )
#if defined( VK_EXT_descriptor_buffer )
    LOAD_VK_FUNC( vkGetDescriptorSetLayoutSizeEXT );
    LOAD_VK_FUNC( vkGetDescriptorSetLayoutBindingOffsetEXT );
    LOAD_VK_FUNC( vkGetDescriptorEXT );
    LOAD_VK_FUNC( vkCmdBindDescriptorBuffersEXT );
    LOAD_VK_FUNC( vkCmdSetDescriptorBufferOffsetsEXT );
    LOAD_VK_FUNC( vkCmdBindDescriptorBufferEmbeddedSamplersEXT );
    LOAD_VK_FUNC( vkGetBufferOpaqueCaptureDescriptorDataEXT );
    LOAD_VK_FUNC( vkGetImageOpaqueCaptureDescriptorDataEXT );
    LOAD_VK_FUNC( vkGetImageViewOpaqueCaptureDescriptorDataEXT );
    LOAD_VK_FUNC( vkGetSamplerOpaqueCaptureDescriptorDataEXT );
    LOAD_VK_FUNC( vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT );
#endif // #if defined( VK_EXT_descriptor_buffer )
#endif // #if USING( PG_DESCRIPTOR_BUFFER )

    LOAD_VK_FUNC( vkCmdDrawMeshTasksIndirectCountEXT );
    LOAD_VK_FUNC( vkCmdDrawMeshTasksIndirectEXT );
    LOAD_VK_FUNC( vkCmdDrawMeshTasksEXT );
}

u32 FindMemoryType( u32 typeFilter, VkMemoryPropertyFlags properties )
{
    auto memProperties = rg.physicalDevice.GetMemoryProperties();
    for ( u32 i = 0; i < memProperties.memoryTypeCount; ++i )
    {
        if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
        {
            return i;
        }
    }

    PG_ASSERT( false );

    return ~0u;
}

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( rg.physicalDevice, format, &props );
    return ( props.optimalTilingFeatures & requestedSupport ) == requestedSupport;
}

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels, u32 layers )
{
    PG_ASSERT( layers == 1 || layers == 6 );
    VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    info.image                 = image;
    info.format                = format;
    info.subresourceRange      = ImageSubresourceRange( aspectFlags, 0, mipLevels, 0, layers );
    info.viewType              = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

    VkImageView view;
    VK_CHECK( vkCreateImageView( rg.device, &info, nullptr, &view ) );

    return view;
}

VkImageSubresourceRange ImageSubresourceRange(
    VkImageAspectFlags aspectFlags, u32 baseMipLevel, u32 levelCount, u32 baseArrayLayer, u32 layerCount )
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
