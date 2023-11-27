#pragma once

#include "renderer/graphics_api/framebuffer.hpp"
#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/graphics_api/render_pass.hpp"
#include "renderer/vulkan.hpp"

namespace PG
{
namespace Gfx
{
    class DescriptorSet;

    enum CommandBufferUsageBits
    {
        COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT = 1 << 0,
    };

    typedef uint32_t CommandBufferUsage;

    class CommandBuffer
    {
        friend class CommandPool;
    public:
        CommandBuffer() = default;

        operator bool() const;
        VkCommandBuffer GetHandle() const;

        void Free();
        void BeginRecording( CommandBufferUsage flags = 0 ) const;
        void EndRecording() const;
        void BeginRenderPass( const RenderPass* renderPass, const Framebuffer& framebuffer ) const;
        void EndRenderPass() const;
        void BindPipeline( Pipeline* pipeline );
        void BindDescriptorSet( const DescriptorSet& set, uint32_t setNumber ) const;
        void BindDescriptorSets( uint32_t numSets, DescriptorSet* sets, uint32_t firstSet ) const;
        void BindVertexBuffer( const Buffer& buffer, size_t offset = 0, uint32_t firstBinding = 0 ) const;
        void BindVertexBuffers( uint32_t numBuffers, const Buffer* buffers, size_t* offsets, uint32_t firstBinding = 0 ) const;
        void BindIndexBuffer( const Buffer& buffer, IndexType indexType = IndexType::UNSIGNED_INT, size_t offset = 0 ) const;
        void PipelineBufferBarrier( PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, const VkBufferMemoryBarrier& barrier ) const;
        void PipelineImageBarrier( PipelineStageFlags srcStageMask, PipelineStageFlags dstStageMask, const VkImageMemoryBarrier& barrier ) const;
        void SetViewport( const Viewport& viewport ) const;
        void SetScissor( const Scissor& scissor ) const;
        void SetDepthBias( float constant, float clamp, float slope ) const;
        
        void PushConstants( uint32_t offset, uint32_t size, void* data ) const;

        void CopyBuffer( const Buffer& dst, const Buffer& src, size_t size = VK_WHOLE_SIZE, size_t srcOffset = 0, size_t dstOffset = 0 ) const;
        void BlitImage( VkImage src, ImageLayout srcLayout, VkImage dst, ImageLayout dstLayout, const VkImageBlit& region ) const;
        void TransitionImageLayout( VkImage image, ImageLayout oldLayout, ImageLayout newLayout, VkImageSubresourceRange subresourceRange,
            PipelineStageFlags srcStageMask = PipelineStageFlags::ALL_COMMANDS_BIT, PipelineStageFlags dstStageMask = PipelineStageFlags::ALL_COMMANDS_BIT ) const;

        void Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstInstance = 0 ) const;
        void DrawIndexed( uint32_t firstIndex, uint32_t indexCount, int vertexOffset = 0, uint32_t firstInstance = 0, uint32_t instanceCount = 1 ) const;
        void Dispatch( uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ ) const;

    private:
        VkDevice m_device        = VK_NULL_HANDLE;
        VkCommandPool m_pool     = VK_NULL_HANDLE;
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
        Pipeline* m_boundPipeline;
    };

    typedef enum CommandPoolCreateFlagBits
    {
        COMMAND_POOL_TRANSIENT            = 1 << 0,
        COMMAND_POOL_RESET_COMMAND_BUFFER = 1 << 1,
    } CommandPoolCreateFlagBits;

    typedef uint32_t CommandPoolCreateFlags;

    enum class CommandPoolQueueFamily
    {
        GRAPHICS,
        COMPUTE
    };

    class CommandPool
    {
        friend class Device;
    public:
        CommandPool() = default;

        void Free();
        CommandBuffer NewCommandBuffer( const std::string& name = "" );
        operator bool() const;
        VkCommandPool GetHandle() const;


    private:
        VkCommandPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device      = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace PG
