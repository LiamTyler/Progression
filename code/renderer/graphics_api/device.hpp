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
#include "renderer/r_gpu_upload_manager.hpp"
#include "vk-bootstrap/VkBootstrap.h"

namespace PG::Gfx
{

class Device
{
public:
    Device() = default;

    bool Create( const vkb::Device& vkbDevice );
    void Free();

    void Submit( QueueType queue, const CommandBuffer& cmdBuf, const VkSemaphoreSubmitInfo* waitSemaphoreInfo = nullptr,
        const VkSemaphoreSubmitInfo* signalSemaphoreInfo = nullptr, Fence* fence = nullptr ) const;
    void WaitForIdle() const;
    CommandPool NewCommandPool( const CommandPoolCreateInfo& info, std::string_view name = "" ) const;
    AccelerationStructure NewAccelerationStructure( AccelerationStructureType type, size_t size ) const;

    Buffer NewBuffer( const BufferCreateInfo& info, std::string_view name = "" ) const;
    Buffer NewStagingBuffer( u64 size ) const;
    Texture NewTexture( const TextureCreateInfo& info, std::string_view name = "" ) const;

    void AddUploadRequest( const Buffer& buffer, const void* data, u64 size, u64 offset = 0 );
    void AddUploadRequest( const Texture& texture, const void* data );
    void FlushUploadRequests();
    void AcquirePendingTransfers();

    Sampler NewSampler( const SamplerCreateInfo& info ) const;
    Fence NewFence( bool signaled = false, std::string_view name = "" ) const;
    Semaphore NewSemaphore( std::string_view name = "" ) const;

    // returns false if present failed because the swapchain needs to be recreated
    bool Present( const Swapchain& swapChain, const Semaphore& waitSemaphore ) const;

    void CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips = true ) const;

    operator bool() const;
    operator VkDevice() const { return m_handle; }
    VkDevice GetHandle() const;
    Queue GetQueue( QueueType type ) const;
    VmaAllocator GetAllocator() const;

private:
    VkDevice m_handle = VK_NULL_HANDLE;
    Queue m_queues[Underlying( QueueType::COUNT )];
    VmaAllocator m_vmaAllocator;
    UploadManager m_uploadBufferManager;
};

} // namespace PG::Gfx
