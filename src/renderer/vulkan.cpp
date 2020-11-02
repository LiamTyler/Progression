#include "renderer/vulkan.hpp"
#include "core/assert.hpp"
#include "renderer/r_globals.hpp"

namespace PG
{
namespace Gfx
{

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    auto memProperties = r_globals.physicalDevice.GetMemoryProperties();
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


void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layers )
{
    CommandBuffer cmdBuf = r_globals.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time TransitionImageLayout" );
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = layers;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = 0;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;
    if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        PG_ASSERT( false, "The transition barriers are unknown for the given old and new layouts" );
    }

    cmdBuf.PipelineImageBarrier( srcStage, dstStage, barrier );

    cmdBuf.EndRecording();
    r_globals.device.Submit( cmdBuf );
    r_globals.device.WaitForIdle();
    cmdBuf.Free();
}


bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( r_globals.physicalDevice.GetHandle(), format, &props );
    return ( props.optimalTilingFeatures & requestedSupport ) == requestedSupport;
}


VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, uint32_t layers )
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image        = image;
    PG_ASSERT( layers == 1 || layers == 6 );
    viewCreateInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
    if ( layers == 6 )
    {
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    viewCreateInfo.format       = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // specify image purpose and which part to access
    viewCreateInfo.subresourceRange.aspectMask     = aspectFlags;
    viewCreateInfo.subresourceRange.baseMipLevel   = 0;
    viewCreateInfo.subresourceRange.levelCount     = mipLevels;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount     = layers;

    VkImageView view;
    VK_CHECK_RESULT( vkCreateImageView( r_globals.device.GetHandle(), &viewCreateInfo, nullptr, &view ) );

    return view;
}

} // namespace Gfx
} // namespace PG
