#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "shared/assert.hpp"

namespace PG::Gfx
{

CommandBuffer::operator bool() const { return m_handle != VK_NULL_HANDLE; }
CommandBuffer::operator VkCommandBuffer() const { return m_handle; }
VkCommandBuffer CommandBuffer::GetHandle() const { return m_handle; }

void CommandBuffer::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkFreeCommandBuffers( rg.device, m_pool, 1, &m_handle );
    m_handle = VK_NULL_HANDLE;
}

void CommandBuffer::Reset() const { VK_CHECK( vkResetCommandBuffer( m_handle, 0 ) ); }

void CommandBuffer::BeginRecording( CommandBufferUsage flags ) const
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = PGToVulkanCommandBufferUsage( flags );
    beginInfo.pInheritanceInfo         = nullptr;
    VK_CHECK( vkBeginCommandBuffer( m_handle, &beginInfo ) );
}

void CommandBuffer::EndRecording() const { VK_CHECK( vkEndCommandBuffer( m_handle ) ); }

void CommandBuffer::BindPipeline( Pipeline* pipeline )
{
    PG_ASSERT( pipeline );
    vkCmdBindPipeline( m_handle, pipeline->GetPipelineBindPoint(), pipeline->GetHandle() );
    m_boundPipeline = pipeline;
}

void CommandBuffer::BindGlobalDescriptors() const
{
#if USING( PG_DESCRIPTOR_BUFFER )
    VkDescriptorBufferBindingInfoEXT pBindingInfos{};
    pBindingInfos.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    pBindingInfos.address = GetGlobalDescriptorBuffer().buffer.GetDeviceAddress();
    pBindingInfos.usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    vkCmdBindDescriptorBuffersEXT( m_handle, 1, &pBindingInfos );

    u32 bufferIndex           = 0; // index into pBindingInfos for vkCmdBindDescriptorBuffersEXT?
    VkDeviceSize bufferOffset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(
        m_handle, m_boundPipeline->GetPipelineBindPoint(), m_boundPipeline->GetLayoutHandle(), 0, 1, &bufferIndex, &bufferOffset );
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
    vkCmdBindDescriptorSets( m_handle, m_boundPipeline->GetPipelineBindPoint(), m_boundPipeline->GetLayoutHandle(), 0, 1,
        &GetGlobalDescriptorSet(), 0, nullptr );
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )
}

void CommandBuffer::BindVertexBuffer( const Buffer& buffer, size_t offset, u32 firstBinding ) const
{
    VkBuffer vkBuffer = buffer;
    vkCmdBindVertexBuffers( m_handle, firstBinding, 1, &vkBuffer, &offset );
}

void CommandBuffer::BindVertexBuffers( u32 numBuffers, const Buffer* buffers, size_t* offsets, u32 firstBinding ) const
{
    std::vector<VkBuffer> vertexBuffers( numBuffers );
    for ( u32 i = 0; i < numBuffers; ++i )
    {
        vertexBuffers[i] = buffers[i];
    }

    vkCmdBindVertexBuffers( m_handle, firstBinding, numBuffers, vertexBuffers.data(), offsets );
}

void CommandBuffer::BindIndexBuffer( const Buffer& buffer, IndexType indexType, size_t offset ) const
{
    vkCmdBindIndexBuffer( m_handle, buffer, offset, PGToVulkanIndexType( indexType ) );
}

void CommandBuffer::PipelineBarrier2( const VkMemoryBarrier2& barrier ) const
{
    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.memoryBarrierCount = 1;
    depInfo.pMemoryBarriers    = &barrier;
    vkCmdPipelineBarrier2( m_handle, &depInfo );
}

void CommandBuffer::PipelineBufferBarrier2( const VkBufferMemoryBarrier2& barrier ) const
{
    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.bufferMemoryBarrierCount = 1;
    depInfo.pBufferMemoryBarriers    = &barrier;

    vkCmdPipelineBarrier2( m_handle, &depInfo );
}

void CommandBuffer::PipelineImageBarrier2( const VkImageMemoryBarrier2& imageBarrier ) const
{
    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers    = &imageBarrier;

    vkCmdPipelineBarrier2( m_handle, &depInfo );
}

void CommandBuffer::SetViewport( const Viewport& viewport ) const
{
    VkViewport v;
    v.x        = viewport.x;
    v.y        = viewport.y;
    v.minDepth = viewport.minDepth;
    v.maxDepth = viewport.maxDepth;
    v.width    = viewport.width;
    v.height   = viewport.height;
    vkCmdSetViewport( m_handle, 0, 1, &v );
}

void CommandBuffer::SetScissor( const Scissor& scissor ) const
{
    VkRect2D s;
    s.offset.x      = scissor.x;
    s.offset.y      = scissor.y;
    s.extent.width  = scissor.width;
    s.extent.height = scissor.height;
    vkCmdSetScissor( m_handle, 0, 1, &s );
}

void CommandBuffer::SetDepthBias( f32 constant, f32 clamp, f32 slope ) const { vkCmdSetDepthBias( m_handle, constant, clamp, slope ); }

void CommandBuffer::PushConstants( void* data, u32 size, u32 offset ) const
{
    vkCmdPushConstants( m_handle, m_boundPipeline->GetLayoutHandle(), m_boundPipeline->GetPushConstantShaderStages(), offset, size, data );
}

void CommandBuffer::CopyBuffer( const Buffer& dst, const Buffer& src, size_t size, size_t srcOffset, size_t dstOffset ) const
{
    VkBufferCopy copyRegion;
    copyRegion.dstOffset = dstOffset;
    copyRegion.srcOffset = srcOffset;
    copyRegion.size      = size;
    if ( size == VK_WHOLE_SIZE )
    {
        copyRegion.size = src.GetSize();
    }
    vkCmdCopyBuffer( m_handle, src, dst, 1, &copyRegion );
}

void CommandBuffer::BlitImage( const Texture& dst, const Texture& src, ImageLayout dstLayout, ImageLayout srcLayout,
    VkImageAspectFlags aspect, TextureFilter filter, u8 mip, u8 layer ) const
{
    VkImageBlit2 blitRegion{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
    blitRegion.srcOffsets[1].x = src.GetWidth();
    blitRegion.srcOffsets[1].y = src.GetHeight();
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dst.GetWidth();
    blitRegion.dstOffsets[1].y = dst.GetHeight();
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = aspect;
    blitRegion.srcSubresource.baseArrayLayer = layer;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = mip;

    blitRegion.dstSubresource.aspectMask     = aspect;
    blitRegion.dstSubresource.baseArrayLayer = layer;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.dstSubresource.mipLevel       = mip;

    VkBlitImageInfo2 blitInfo{ VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
    blitInfo.dstImage       = dst.GetImage();
    blitInfo.dstImageLayout = PGToVulkanImageLayout( dstLayout );
    blitInfo.srcImage       = src.GetImage();
    blitInfo.srcImageLayout = PGToVulkanImageLayout( srcLayout );
    blitInfo.filter         = PGToVulkanFilter( filter );
    blitInfo.regionCount    = 1;
    blitInfo.pRegions       = &blitRegion;

    vkCmdBlitImage2( m_handle, &blitInfo );
}

void CommandBuffer::TransitionImageLayout( VkImage image, ImageLayout oldLayout, ImageLayout newLayout, VkPipelineStageFlags2 srcStageMask,
    VkPipelineStageFlags2 dstStageMask, u32 srcQueueFamilyIndex, u32 dstQueueFamilyIndex ) const
{
    VkImageLayout vkOldLayout          = PGToVulkanImageLayout( oldLayout );
    VkImageLayout vkNewLayout          = PGToVulkanImageLayout( newLayout );
    VkImageMemoryBarrier2 imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    imageBarrier.srcStageMask          = srcStageMask;
    imageBarrier.srcAccessMask         = 0;
    imageBarrier.dstStageMask          = dstStageMask;
    imageBarrier.dstAccessMask         = 0;
    imageBarrier.srcQueueFamilyIndex   = srcQueueFamilyIndex;
    imageBarrier.dstQueueFamilyIndex   = dstQueueFamilyIndex;

    imageBarrier.oldLayout = vkOldLayout;
    imageBarrier.newLayout = vkNewLayout;
    imageBarrier.image     = image;

    VkImageAspectFlags aspectMask = 0;
    if ( ImageLayoutHasDepthAspect( newLayout ) )
        aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    if ( ImageLayoutHasDepthAspect( newLayout ) )
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    if ( !aspectMask )
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    imageBarrier.subresourceRange = ImageSubresourceRange( aspectMask );

    // https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
    // Source access mask controls actions that have to be finished on the old layout before it will be transitioned to the new layout
    switch ( vkOldLayout )
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageBarrier.srcAccessMask = VK_ACCESS_2_NONE;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageBarrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        // Image was already presented, and should not have to wait for anything before being written to
        imageBarrier.srcAccessMask = 0;
        break;
    default:
        // Other source layouts aren't handled (yet)
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        break;
    }

    // Destination access mask controls the dependency for the new image layout
    switch ( vkNewLayout )
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageBarrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageBarrier.dstAccessMask = imageBarrier.dstAccessMask | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if ( imageBarrier.srcAccessMask == 0 )
        {
            imageBarrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
        }
        imageBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        // Not sure what presentation counts as?
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;
        break;
    default: imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT; break;
    }

    PipelineImageBarrier2( imageBarrier );
}

void CommandBuffer::Draw( u32 firstVert, u32 vertCount, u32 instanceCount, u32 firstInstance ) const
{
    vkCmdDraw( m_handle, vertCount, instanceCount, firstVert, firstInstance );
}

void CommandBuffer::DrawIndexed( u32 firstIndex, u32 indexCount, i32 vertexOffset, u32 firstInstance, u32 instanceCount ) const
{
    vkCmdDrawIndexed( m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}

void CommandBuffer::Dispatch( u32 groupsX, u32 groupsY, u32 groupsZ ) const
{
    PG_DBG_ASSERT( m_boundPipeline && m_boundPipeline->GetPipelineType() == PipelineType::COMPUTE );
    vkCmdDispatch( m_handle, groupsX, groupsY, groupsZ );
}

void CommandBuffer::Dispatch_AutoSized( u32 itemsX, u32 itemsY, u32 itemsZ ) const
{
    PG_DBG_ASSERT( m_boundPipeline && m_boundPipeline->GetPipelineType() == PipelineType::COMPUTE );
    uvec3 gSize = m_boundPipeline->GetWorkgroupSize();
    u32 groupsX = ( itemsX + gSize.x - 1 ) / gSize.x;
    u32 groupsY = ( itemsY + gSize.y - 1 ) / gSize.y;
    u32 groupsZ = ( itemsZ + gSize.z - 1 ) / gSize.z;
    vkCmdDispatch( m_handle, groupsX, groupsY, groupsZ );
}

void CommandBuffer::DrawMeshTasks( u32 groupsX, u32 groupsY, u32 groupsZ ) const
{
    PG_DBG_ASSERT( m_boundPipeline && m_boundPipeline->GetPipelineType() == PipelineType::GRAPHICS );
    vkCmdDrawMeshTasksEXT( m_handle, groupsX, groupsY, groupsZ );
}

void CommandBuffer::DrawMeshTasksIndirect( const Buffer& buffer, u32 drawCount, u64 offset, u32 stride ) const
{
    PG_DBG_ASSERT( m_boundPipeline && m_boundPipeline->GetPipelineType() == PipelineType::GRAPHICS );
    vkCmdDrawMeshTasksIndirectEXT( m_handle, buffer, offset, drawCount, stride );
}

void CommandBuffer::DrawMeshTasksIndirectCount(
    const Buffer& indirectCmdsBuffer, u64 indirectOffset, const Buffer& countBuffer, u64 countOffset, u32 maxDrawCount, u32 stride ) const
{
    PG_DBG_ASSERT( m_boundPipeline && m_boundPipeline->GetPipelineType() == PipelineType::GRAPHICS );
    vkCmdDrawMeshTasksIndirectCountEXT( m_handle, indirectCmdsBuffer, indirectOffset, countBuffer, countOffset, maxDrawCount, stride );
}

void CommandPool::Free()
{
    if ( m_handle != VK_NULL_HANDLE )
    {
        vkDestroyCommandPool( rg.device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }
}

CommandBuffer CommandPool::NewCommandBuffer( std::string_view name )
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_handle;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = 1;

    CommandBuffer buf;
    buf.m_pool = m_handle;
    VkCommandBuffer handle;
    VK_CHECK( vkAllocateCommandBuffers( rg.device, &allocInfo, &handle ) );
    buf.m_handle = handle;
    PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( buf, name );

    return buf;
}

VkCommandPool CommandPool::GetHandle() const { return m_handle; }
CommandPool::operator bool() const { return m_handle != VK_NULL_HANDLE; }
CommandPool::operator VkCommandPool() const { return m_handle; }

} // namespace PG::Gfx
