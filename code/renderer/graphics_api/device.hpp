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
    VkQueue queue        = VK_NULL_HANDLE;
    uint32_t familyIndex = ~0u;
    uint32_t queueIndex  = ~0u;

    operator VkQueue() const { return queue; }
    operator uint32_t() const { return familyIndex; }
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
    Buffer NewStagingBuffer( size_t size ) const;
    Texture NewTexture( const TextureCreateInfo& desc, std::string_view name = "" ) const;
    Texture NewTextureWithData( TextureCreateInfo& desc, void* data, std::string_view name = "" ) const;

    Sampler NewSampler( const SamplerCreateInfo& desc ) const;
    Fence NewFence( bool signaled = false, std::string_view name = "" ) const;
    Semaphore NewSemaphore( std::string_view name = "" ) const;
    Pipeline NewGraphicsPipeline( const PipelineCreateInfo& desc, std::string_view name = "" ) const;
    Pipeline NewComputePipeline( Shader* shader, std::string_view name = "" ) const;

    // returns false if present failed because the swapchain needs to be recreated
    bool Present( const Swapchain& swapChain, const Semaphore& waitSemaphore ) const;

    void CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips = true ) const;

    operator bool() const;
    operator VkDevice() const { return m_handle; }
    VkDevice GetHandle() const;
    Queue GetQueue() const;
    VmaAllocator GetAllocator() const;

private:
    VkDevice m_handle = VK_NULL_HANDLE;
    Queue m_queue     = {};
    VmaAllocator m_vmaAllocator;
};

} // namespace PG::Gfx
