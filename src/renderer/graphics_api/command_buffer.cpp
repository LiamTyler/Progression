#include "renderer/graphics_api/command_buffer.hpp"
#include "core/assert.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"

namespace PG
{
namespace Gfx
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
        beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags            = PGToVulkanCommandBufferUsage( flags );
        beginInfo.pInheritanceInfo = nullptr;
        VK_CHECK_RESULT( vkBeginCommandBuffer( m_handle, &beginInfo ) );
    }


    void CommandBuffer::EndRecording() const
    {
        VK_CHECK_RESULT( vkEndCommandBuffer( m_handle ) );
    }


    void CommandBuffer::BeginRenderPass( const RenderPass& renderPass, const Framebuffer& framebuffer ) const
    {
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = renderPass.GetHandle();
        renderPassInfo.framebuffer       = framebuffer.GetHandle();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { framebuffer.GetWidth(), framebuffer.GetHeight() };

        VkClearValue clearValues[9] = {};
        size_t i = 0;
        const auto& colorA = renderPass.desc.colorAttachmentDescriptors;
        for ( ; i < colorA.size() && colorA[i].format != PixelFormat::INVALID; ++i )
        {
            const auto& col = renderPass.desc.colorAttachmentDescriptors[i].clearColor;
            clearValues[i].color = { col.r, col.g, col.b, col.a };
        }
        if ( renderPass.desc.depthAttachmentDescriptor.format != PixelFormat::INVALID )
        {
            clearValues[i].depthStencil = { renderPass.desc.depthAttachmentDescriptor.clearValue, 0 };
            ++i;
        }

        renderPassInfo.clearValueCount = static_cast< uint32_t >( i );
        renderPassInfo.pClearValues    = clearValues;

        vkCmdBeginRenderPass( m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    }


    void CommandBuffer::EndRenderPass() const
    {
        vkCmdEndRenderPass( m_handle );
    }


    void CommandBuffer::BindPipeline( const Pipeline& pipeline ) const
    {
        vkCmdBindPipeline( m_handle, pipeline.GetPipelineBindPoint(), pipeline.GetHandle() );
    }


    void CommandBuffer::BindDescriptorSets( uint32_t numSets, DescriptorSet* sets, const Pipeline& pipeline, uint32_t firstSet ) const
    {
        vkCmdBindDescriptorSets( m_handle, pipeline.GetPipelineBindPoint(), pipeline.GetLayoutHandle(), firstSet, numSets, (VkDescriptorSet*) sets, 0, nullptr );
    }


    void CommandBuffer::BindVertexBuffer( const Buffer& buffer, size_t offset, uint32_t firstBinding ) const
    {
        VkBuffer vertexBuffers[] = { buffer.GetHandle() };
        vkCmdBindVertexBuffers( m_handle, firstBinding, 1, vertexBuffers, &offset );
    }


    void CommandBuffer::BindVertexBuffers( uint32_t numBuffers, const Buffer* buffers, size_t* offsets, uint32_t firstBinding ) const
    {
        std::vector< VkBuffer > vertexBuffers( numBuffers );
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


    void CommandBuffer::PipelineBufferBarrier( VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const VkBufferMemoryBarrier& barrier ) const
    {
        vkCmdPipelineBarrier( m_handle, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr );
    }


    void CommandBuffer::PipelineImageBarrier( VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const VkImageMemoryBarrier& barrier ) const
    {
        vkCmdPipelineBarrier( m_handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );
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


    void CommandBuffer::PushConstants( const Pipeline& pipeline, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, void* data ) const
    {
        vkCmdPushConstants( m_handle, pipeline.GetLayoutHandle(), stageFlags, offset, size, data );
    }


    void CommandBuffer::CopyBuffer( const Buffer& dst, const Buffer& src, size_t size = VK_WHOLE_SIZE, size_t srcOffset = 0, size_t dstOffset = 0 ) const
    {
        VkBufferCopy copyRegion;
        copyRegion.dstOffset = dstOffset;
        copyRegion.srcOffset = srcOffset;
        copyRegion.size = size;
        vkCmdCopyBuffer( m_handle, src.GetHandle(), dst.GetHandle(), 1, &copyRegion );
    }
    

    void CommandBuffer::Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount, uint32_t firstInstance ) const
    {
        vkCmdDraw( m_handle, vertCount, instanceCount, firstVert, firstInstance );
    }


    void CommandBuffer::DrawIndexed( uint32_t firstIndex, uint32_t indexCount, int vertexOffset, uint32_t firstInstance, uint32_t instanceCount ) const
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
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_handle;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
    
        CommandBuffer buf;
        buf.m_device = m_device;
        buf.m_pool   = m_handle;
        VkCommandBuffer handle;
        if ( vkAllocateCommandBuffers( m_device, &allocInfo, &handle ) != VK_SUCCESS )
        {
            PG_ASSERT( false, "Could not allocate command buffers\n" );
            return buf;
        }
        buf.m_handle = handle;
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( buf, name ) );

        return buf;
    }


    VkCommandPool CommandPool::GetHandle() const { return m_handle; }
    CommandPool::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace Gfx
} // namespace PG
