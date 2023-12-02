#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "shared/assert.hpp"

namespace PG::Gfx
{

CommandBuffer::operator bool() const { return m_handle != VK_NULL_HANDLE; }
VkCommandBuffer CommandBuffer::GetHandle() const { return m_handle; }

void CommandBuffer::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkFreeCommandBuffers( m_device, m_pool, 1, &m_handle );
    m_handle = VK_NULL_HANDLE;
}

void CommandBuffer::BeginRecording( CommandBufferUsage flags ) const
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = PGToVulkanCommandBufferUsage( flags );
    beginInfo.pInheritanceInfo         = nullptr;
    VK_CHECK_RESULT( vkBeginCommandBuffer( m_handle, &beginInfo ) );
}

void CommandBuffer::EndRecording() const { VK_CHECK_RESULT( vkEndCommandBuffer( m_handle ) ); }

void CommandBuffer::BeginRenderPass( const RenderPass* renderPass, const Framebuffer& framebuffer ) const
{
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass            = renderPass->GetHandle();
    renderPassInfo.framebuffer           = framebuffer.GetHandle();
    renderPassInfo.renderArea.offset     = { 0, 0 };
    renderPassInfo.renderArea.extent     = { framebuffer.GetWidth(), framebuffer.GetHeight() };

    VkClearValue clearValues[9] = {};
    uint8_t attachmentIndex     = 0;
    for ( ; attachmentIndex < renderPass->desc.numColorAttachments; ++attachmentIndex )
    {
        const glm::vec4& col               = renderPass->desc.colorAttachmentDescriptors[attachmentIndex].clearColor;
        clearValues[attachmentIndex].color = { col.r, col.g, col.b, col.a };
    }
    if ( renderPass->desc.numDepthAttachments > 0 )
    {
        clearValues[attachmentIndex].depthStencil = { renderPass->desc.depthAttachmentDescriptor.clearValue, 0 };
        ++attachmentIndex;
    }

    renderPassInfo.clearValueCount = attachmentIndex;
    renderPassInfo.pClearValues    = clearValues;

    vkCmdBeginRenderPass( m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void CommandBuffer::EndRenderPass() const { vkCmdEndRenderPass( m_handle ); }

void CommandBuffer::BindPipeline( Pipeline* pipeline )
{
    vkCmdBindPipeline( m_handle, pipeline->GetPipelineBindPoint(), pipeline->GetHandle() );
    m_boundPipeline = pipeline;
}

void CommandBuffer::BindDescriptorSet( const DescriptorSet& set, uint32_t setNumber ) const
{
    vkCmdBindDescriptorSets( m_handle, m_boundPipeline->GetPipelineBindPoint(), m_boundPipeline->GetLayoutHandle(), setNumber, 1,
        (VkDescriptorSet*)&set, 0, nullptr );
}

void CommandBuffer::BindDescriptorSets( uint32_t numSets, DescriptorSet* sets, uint32_t firstSet ) const
{
    vkCmdBindDescriptorSets( m_handle, m_boundPipeline->GetPipelineBindPoint(), m_boundPipeline->GetLayoutHandle(), firstSet, numSets,
        (VkDescriptorSet*)sets, 0, nullptr );
}

void CommandBuffer::BindVertexBuffer( const Buffer& buffer, size_t offset, uint32_t firstBinding ) const
{
    VkBuffer vertexBuffers[] = { buffer.GetHandle() };
    vkCmdBindVertexBuffers( m_handle, firstBinding, 1, vertexBuffers, &offset );
}

void CommandBuffer::BindVertexBuffers( uint32_t numBuffers, const Buffer* buffers, size_t* offsets, uint32_t firstBinding ) const
{
    std::vector<VkBuffer> vertexBuffers( numBuffers );
    for ( uint32_t i = 0; i < numBuffers; ++i )
    {
        vertexBuffers[i] = buffers[i].GetHandle();
    }

    vkCmdBindVertexBuffers( m_handle, firstBinding, numBuffers, vertexBuffers.data(), offsets );
}

void CommandBuffer::BindIndexBuffer( const Buffer& buffer, IndexType indexType, size_t offset ) const
{
    vkCmdBindIndexBuffer( m_handle, buffer.GetHandle(), offset, PGToVulkanIndexType( indexType ) );
}

void CommandBuffer::PipelineBufferBarrier(
    PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& barrier ) const
{
    vkCmdPipelineBarrier( m_handle, PGToVulkanPipelineStageFlags( srcStageMask ), PGToVulkanPipelineStageFlags( dstStageMask ), 0, 0,
        nullptr,      // memory barriers
        1, &barrier,  // buffer memory barriers
        0, nullptr ); // image memory barriers
}

void CommandBuffer::PipelineImageBarrier(
    PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, const VkImageMemoryBarrier& barrier ) const
{
    vkCmdPipelineBarrier( m_handle, PGToVulkanPipelineStageFlags( srcStageMask ), PGToVulkanPipelineStageFlags( dstStageMask ), 0, 0,
        nullptr,       // memory barriers
        0, nullptr,    // buffer memory barriers
        1, &barrier ); // image memory barriers
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

void CommandBuffer::SetDepthBias( float constant, float clamp, float slope ) const
{
    vkCmdSetDepthBias( m_handle, constant, clamp, slope );
}

void CommandBuffer::PushConstants( uint32_t offset, uint32_t size, void* data ) const
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
        copyRegion.size = src.GetLength();
    }
    vkCmdCopyBuffer( m_handle, src.GetHandle(), dst.GetHandle(), 1, &copyRegion );
}

void CommandBuffer::BlitImage( VkImage src, ImageLayout srcLayout, VkImage dst, ImageLayout dstLayout, const VkImageBlit& region ) const
{
    vkCmdBlitImage(
        m_handle, src, PGToVulkanImageLayout( srcLayout ), dst, PGToVulkanImageLayout( dstLayout ), 1, &region, VK_FILTER_LINEAR );
}

void CommandBuffer::TransitionImageLayout( VkImage image, ImageLayout oldLayout, ImageLayout newLayout,
    VkImageSubresourceRange subresourceRange, PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask ) const
{
    VkImageLayout vkOldLayout               = PGToVulkanImageLayout( oldLayout );
    VkImageLayout vkNewLayout               = PGToVulkanImageLayout( newLayout );
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout            = vkOldLayout;
    imageMemoryBarrier.newLayout            = vkNewLayout;
    imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image                = image;
    imageMemoryBarrier.subresourceRange     = subresourceRange;

    // https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
    // Source access mask controls actions that have to be finished on the old layout before it will be transitioned to the new layout
    switch ( vkOldLayout )
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        // Image layout is undefined (or does not matter)
        // Only valid as initial layout
        // No flags required, listed only for completeness
        imageMemoryBarrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        // Image is preinitialized
        // Only valid as initial layout for linear images, preserves memory contents
        // Make sure host writes have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image is a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image is a depth/stencil attachment
        // Make sure any writes to the depth/stencil buffer have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image is a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image is a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image is read by a shader
        // Make sure any shader reads from the image have been finished
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        // Image was already presented, and should not have to wait for anything before being written to
        imageMemoryBarrier.srcAccessMask = 0;
        break;
    default:
        // Other source layouts aren't handled (yet)
        PG_ASSERT( false, "TransitionImageLayout: unknown oldLayout %d", vkOldLayout );
        break;
    }

    // Destination access mask controls the dependency for the new image layout
    switch ( vkNewLayout )
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Image will be used as a transfer destination
        // Make sure any writes to the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Image will be used as a transfer source
        // Make sure any reads from the image have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Image will be used as a color attachment
        // Make sure any writes to the color buffer have been finished
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Image layout will be used as a depth/stencil attachment
        // Make sure any writes to depth/stencil buffer have been finished
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
    case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Image will be read in a shader (sampler, input attachment)
        // Make sure any writes to the image have been finished
        if ( imageMemoryBarrier.srcAccessMask == 0 )
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        // Not sure what presentation counts as?
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        break;
    default:
        // Other source layouts aren't handled (yet)
        PG_ASSERT( false, "TransitionImageLayout: unknown newLayout %d", vkNewLayout );
        break;
    }

    PipelineImageBarrier( srcStageMask, dstStageMask, imageMemoryBarrier );
}

void CommandBuffer::Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount, uint32_t firstInstance ) const
{
    vkCmdDraw( m_handle, vertCount, instanceCount, firstVert, firstInstance );
}

void CommandBuffer::DrawIndexed(
    uint32_t firstIndex, uint32_t indexCount, int vertexOffset, uint32_t firstInstance, uint32_t instanceCount ) const
{
    vkCmdDrawIndexed( m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
}

void CommandBuffer::Dispatch( uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ ) const
{
    vkCmdDispatch( m_handle, groupsX, groupsY, groupsZ );
}

void CommandPool::Free()
{
    if ( m_handle != VK_NULL_HANDLE )
    {
        vkDestroyCommandPool( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }
}

CommandBuffer CommandPool::NewCommandBuffer( const std::string& name )
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool                 = m_handle;
    allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount          = 1;

    CommandBuffer buf;
    buf.m_device = m_device;
    buf.m_pool   = m_handle;
    VkCommandBuffer handle;
    if ( vkAllocateCommandBuffers( m_device, &allocInfo, &handle ) != VK_SUCCESS )
    {
        PG_ASSERT( false, "Could not allocate command buffers" );
        return buf;
    }
    buf.m_handle = handle;
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( buf, name ) );

    return buf;
}

VkCommandPool CommandPool::GetHandle() const { return m_handle; }
CommandPool::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace PG::Gfx
