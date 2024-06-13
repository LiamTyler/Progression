#pragma once

#include "renderer/graphics_api/acceleration_structure.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/physical_device.hpp"
#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/graphics_api/sampler.hpp"
#include "renderer/graphics_api/swapchain.hpp"
#include "renderer/graphics_api/synchronization.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "vk-bootstrap/VkBootstrap.h"

namespace PG::Gfx
{

struct Queue
{
    VkQueue queue   = VK_NULL_HANDLE;
    u32 familyIndex = ~0u;
    u32 queueIndex  = ~0u;

    operator VkQueue() const { return queue; }
};

enum class UploadCommandType : u8
{
    BUFFER_UPLOAD,
    TEXTURE_UPLOAD,
    TEXTURE_TRANSITION,
};

struct BufferUploadRequest
{
    u64 dstOffset;
    u32 size;
    u32 offsetInStagingBuffer;
};

struct TextureUploadRequest
{
    std::vector<VkBufferImageCopy> copies;
};

struct TextureTransitionRequest
{
    ImageLayout prevLayout;
    ImageLayout newLayout;
};

struct UploadRequest
{
    UploadRequest() = default;
    UploadRequest( UploadCommandType inType ) : type( inType ) {}

    UploadCommandType type;
    union
    {
        VkImage image;
        VkBuffer buffer;
    };
    const void* data;
    TextureTransitionRequest texTransitionReq;
    BufferUploadRequest bufferReq;
    TextureUploadRequest texReq;
};

struct UploadRequest2
{
    UploadRequest2() = default;
    UploadRequest2( UploadCommandType inType ) : type( inType ) {}

    UploadCommandType type;
    union
    {
        VkImage image;
        VkBuffer buffer;
    };
    const char* data;
    u32 size = 0;
    u64 offset;

    TextureTransitionRequest texTransitionReq;
    // TextureUploadRequest texReq;
    VkBufferImageCopy texReq;
};

struct UploadBufferManager
{
    static constexpr u32 NUM_BUFFERS = 2;
    static constexpr u32 BUFF_SIZE   = 64 * 1024 * 1024;

    Buffer stagingBuffers[NUM_BUFFERS];
    u64 currentSizes[NUM_BUFFERS];
    std::vector<UploadRequest2> requests[NUM_BUFFERS];
    Fence fences[NUM_BUFFERS];
    CommandPool cmdPool;
    CommandBuffer cmdBufs[NUM_BUFFERS];

    u32 currentBufferIdx;

    void Init();
    void Free();

    void AddRequest( const UploadRequest2& req );
    void StartUploads();
    void SwapBuffers();
    void FlushAll();
};

class Device
{
public:
    Device() = default;

    bool Create( const vkb::Device& vkbDevice );
    void Free();

    void Submit( const CommandBuffer& cmdBuf, const VkSemaphoreSubmitInfo* waitSemaphoreInfo = nullptr,
        const VkSemaphoreSubmitInfo* signalSemaphoreInfo = nullptr, Fence* fence = nullptr ) const;
    void WaitForIdle() const;
    CommandPool NewCommandPool( CommandPoolCreateFlags flags = 0, std::string_view name = "" ) const;
    AccelerationStructure NewAccelerationStructure( AccelerationStructureType type, size_t size ) const;

    Buffer NewBuffer( const BufferCreateInfo& info, std::string_view name = "" ) const;
    Buffer NewStagingBuffer( u64 size ) const;
    Texture NewTexture( const TextureCreateInfo& desc, std::string_view name = "" ) const;

    void AddUploadRequest( const Buffer& buffer, const void* data, u64 size, u64 offset = 0 );
    void AddUploadRequest( const Texture& texture, const void* data );
    void FlushUploadRequests();

    Sampler NewSampler( const SamplerCreateInfo& desc ) const;
    Fence NewFence( bool signaled = false, std::string_view name = "" ) const;
    Semaphore NewSemaphore( std::string_view name = "" ) const;

    // returns false if present failed because the swapchain needs to be recreated
    bool Present( const Swapchain& swapChain, const Semaphore& waitSemaphore ) const;

    void CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips = true ) const;

    operator bool() const;
    operator VkDevice() const { return m_handle; }
    VkDevice GetHandle() const;
    Queue GetMainQueue() const;
    Queue GetTransferQueue() const;
    VmaAllocator GetAllocator() const;

private:
    std::vector<UploadRequest> m_uploadRequests;
    size_t m_currentStagingSize;
    Buffer m_stagingBuffer;
    UploadBufferManager m_uploadBufferManager;

    VkDevice m_handle     = VK_NULL_HANDLE;
    Queue m_mainQueue     = {};
    Queue m_transferQueue = {};
    VmaAllocator m_vmaAllocator;
};

} // namespace PG::Gfx
