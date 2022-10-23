#pragma once

#include "renderer/graphics_api/acceleration_structure.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/graphics_api/framebuffer.hpp"
#include "renderer/graphics_api/physical_device.hpp"
#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/graphics_api/sampler.hpp"
#include "renderer/graphics_api/synchronization.hpp"
#include "renderer/graphics_api/texture.hpp"

namespace PG
{
namespace Gfx
{

    struct Queue
    {
        VkQueue  queue       = VK_NULL_HANDLE;
        uint32_t familyIndex = ~0u;
        uint32_t queueIndex  = ~0u;

        operator VkQueue() const { return queue; }
        operator uint32_t() const { return familyIndex; }
    };

    class Device
    {
    public:
        Device() = default;

        bool Create( const PhysicalDevice& physicalDevice, bool headless );
        void Free();
        operator bool() const;

        void Submit( const CommandBuffer& cmdBuf ) const;
        void WaitForIdle() const;
        CommandPool NewCommandPool( CommandPoolCreateFlags flags = 0, CommandPoolQueueFamily family = CommandPoolQueueFamily::GRAPHICS, const std::string& name = "" ) const;
        DescriptorPool NewDescriptorPool( int numPoolSizes, VkDescriptorPoolSize* poolSizes, bool bindless, uint32_t maxSets = 1, const std::string& name = "" ) const;
        bool RegisterDescriptorSetLayout( DescriptorSetLayout& layout, const uint32_t* stagesForBindings ) const;
        void UpdateDescriptorSets( uint32_t count, const VkWriteDescriptorSet* writes ) const;
        void UpdateDescriptorSet( const VkWriteDescriptorSet& write ) const;
        AccelerationStructure NewAccelerationStructure( AccelerationStructureType type, size_t size ) const;
        Buffer NewBuffer( size_t length, BufferType type, MemoryType memoryType, const std::string& name = "" ) const;
        Buffer NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType, const std::string& name = "" ) const;
        Texture NewTexture( const TextureDescriptor& desc, const std::string& name = "" ) const;
        Texture NewTextureFromBuffer( TextureDescriptor& desc, void* data, const std::string& name = "" ) const;
        Sampler NewSampler( const SamplerDescriptor& desc ) const;
        Fence NewFence( bool signaled = false, const std::string& name = "" ) const;
        Semaphore NewSemaphore( const std::string& name = "" ) const;
        Pipeline NewGraphicsPipeline( const PipelineDescriptor& desc, const std::string& name = "" ) const;
        Pipeline NewComputePipeline( Shader* shader, const std::string& name = "" ) const;
        RenderPass NewRenderPass( const RenderPassDescriptor& desc, const std::string& name = "" ) const;
        Framebuffer NewFramebuffer( const std::vector<Texture*>& attachments, const RenderPass& renderPass, const std::string& name = "" ) const;
        Framebuffer NewFramebuffer( const VkFramebufferCreateInfo& info, const std::string& name = "" ) const;
        void SubmitCommandBuffers( int numBuffers, CommandBuffer* cmdBufs ) const;
        void SubmitFrameForPresentation( uint32_t imageIndex ) const;

        void Copy( Buffer dst, Buffer src ) const;
        void CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips = true ) const;

        VkDevice GetHandle() const;
        Queue GetQueue() const;

    private:
        VkDevice m_handle = VK_NULL_HANDLE;
        Queue m_queue = {};
    };

} // namespace Gfx
} // namespace PG
