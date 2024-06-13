#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/graphics_api/texture.hpp"

namespace PG::Gfx
{

enum CommandBufferUsageBits
{
    COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT = 1 << 0,
};

typedef u32 CommandBufferUsage;

class CommandBuffer
{
    friend class CommandPool;

public:
    CommandBuffer() = default;

    operator bool() const;
    operator VkCommandBuffer() const;
    VkCommandBuffer GetHandle() const;

    void Free();
    void Reset() const;
    void BeginRecording( CommandBufferUsage flags = 0 ) const;
    void EndRecording() const;
    void BindPipeline( Pipeline* pipeline );
    void BindGlobalDescriptors() const;
    void BindVertexBuffer( const Buffer& buffer, size_t offset = 0, u32 firstBinding = 0 ) const;
    void BindVertexBuffers( u32 numBuffers, const Buffer* buffers, size_t* offsets, u32 firstBinding = 0 ) const;
    void BindIndexBuffer( const Buffer& buffer, IndexType indexType = IndexType::UNSIGNED_INT, size_t offset = 0 ) const;
    void PipelineBufferBarrier2( const VkBufferMemoryBarrier2& barrier ) const;
    void PipelineImageBarrier2( const VkImageMemoryBarrier2& barrier ) const;
    void SetViewport( const Viewport& viewport ) const;
    void SetScissor( const Scissor& scissor ) const;
    void SetDepthBias( f32 constant, f32 clamp, f32 slope ) const;

    void PushConstants( void* data, u32 size, u32 offset = 0 ) const;
    template <typename T>
    void PushConstants( const T& data, u32 offset = 0 ) const
    {
        vkCmdPushConstants(
            m_handle, m_boundPipeline->GetLayoutHandle(), m_boundPipeline->GetPushConstantShaderStages(), offset, sizeof( T ), &data );
    }

    void CopyBuffer( const Buffer& dst, const Buffer& src, size_t size = VK_WHOLE_SIZE, size_t srcOffset = 0, size_t dstOffset = 0 ) const;
    void BlitImage( VkImage src, ImageLayout srcLayout, VkImage dst, ImageLayout dstLayout, const VkImageBlit& region ) const;
    void TransitionImageLayout( VkImage image, ImageLayout oldLayout, ImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VkPipelineStageFlags2 dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, u32 srcQueueFamilyIndex = 0,
        u32 dstQueueFamilyIndex = 0 ) const;

    void Draw( u32 firstVert, u32 vertCount, u32 instanceCount = 1, u32 firstInstance = 0 ) const;
    void DrawIndexed( u32 firstIndex, u32 indexCount, i32 vertexOffset = 0, u32 firstInstance = 0, u32 instanceCount = 1 ) const;
    void Dispatch( u32 groupsX, u32 groupsY, u32 groupsZ ) const;
    void Dispatch_AutoSized( u32 itemsX, u32 itemsY, u32 itemsZ ) const;
    void DrawMeshTasks( u32 groupsX, u32 groupsY, u32 groupsZ ) const;
    void DrawMeshTasks_AutoSized( u32 itemsX, u32 itemsY, u32 itemsZ ) const;

private:
    VkCommandPool m_pool      = VK_NULL_HANDLE;
    VkCommandBuffer m_handle  = VK_NULL_HANDLE;
    Pipeline* m_boundPipeline = nullptr;
};

typedef enum CommandPoolCreateFlagBits
{
    COMMAND_POOL_TRANSIENT            = 1 << 0,
    COMMAND_POOL_RESET_COMMAND_BUFFER = 1 << 1,
} CommandPoolCreateFlagBits;

typedef u32 CommandPoolCreateFlags;

class CommandPool
{
    friend class Device;

public:
    CommandPool() = default;

    void Free();
    CommandBuffer NewCommandBuffer( std::string_view name = "" );
    operator bool() const;
    operator VkCommandPool() const;
    VkCommandPool GetHandle() const;

private:
    VkCommandPool m_handle = VK_NULL_HANDLE;
};

} // namespace PG::Gfx
