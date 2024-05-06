#include "renderer/graphics_api/device.hpp"
#include "core/feature_defines.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_texture_manager.hpp"
#include "renderer/render_system.hpp"
#include "renderer/vulkan.hpp"
#include "shaders/c_shared/defines.h"
#include "shared/assert.hpp"
#include "shared/bitops.hpp"
#include "shared/logger.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>

namespace PG::Gfx
{

bool Device::Create( const vkb::Device& vkbDevice )
{
    m_handle = vkbDevice.device;
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( *this, "Primary" );

    // TODO: actually ensure this queue supports all the operations we need
    auto queueRet = vkbDevice.get_queue( vkb::QueueType::graphics );
    if ( !queueRet )
    {
        LOG_ERR( "Could not get graphics queue. Error: %s", queueRet.error().message().c_str() );
        return false;
    }
    m_queue.queue       = queueRet.value();
    m_queue.familyIndex = vkbDevice.get_queue_index( vkb::QueueType::graphics ).value();
    m_queue.queueIndex  = 0; // ?
    PG_DEBUG_MARKER_SET_QUEUE_NAME( m_queue.queue, "Primary GCT" );

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = rg.physicalDevice.GetHandle();
    allocatorCreateInfo.device                 = m_handle;
    allocatorCreateInfo.instance               = rg.instance;
    VK_CHECK( vmaCreateAllocator( &allocatorCreateInfo, &m_vmaAllocator ) );

    return true;
}

void Device::Free()
{
    vmaDestroyAllocator( m_vmaAllocator );
    if ( m_handle != VK_NULL_HANDLE )
    {
        vkDestroyDevice( m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }
}

void Device::Submit( const CommandBuffer& cmdBuf, const VkSemaphoreSubmitInfo* waitSemaphoreInfo,
    const VkSemaphoreSubmitInfo* signalSemaphoreInfo, Fence* fence ) const
{
    VkCommandBufferSubmitInfo cmdBufInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    cmdBufInfo.commandBuffer = cmdBuf.GetHandle();
    cmdBufInfo.deviceMask    = 0;

    VkSubmitInfo2 info = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos    = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos    = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos    = &cmdBufInfo;

    VkFence vkFence = fence ? fence->GetHandle() : VK_NULL_HANDLE;
    VK_CHECK( vkQueueSubmit2( m_queue.queue, 1, &info, vkFence ) );
}

void Device::WaitForIdle() const { VK_CHECK( vkQueueWaitIdle( m_queue.queue ) ); }

CommandPool Device::NewCommandPool( CommandPoolCreateFlags flags, const std::string& name ) const
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags                   = PGToVulkanCommandPoolCreateFlags( flags );
    poolInfo.queueFamilyIndex        = m_queue.familyIndex;

    CommandPool cmdPool;
    cmdPool.m_device = m_handle;
    if ( vkCreateCommandPool( m_handle, &poolInfo, nullptr, &cmdPool.m_handle ) != VK_SUCCESS )
    {
        cmdPool.m_handle = VK_NULL_HANDLE;
    }
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( cmdPool, name ) );

    return cmdPool;
}

Fence Device::NewFence( bool signaled, const std::string& name ) const
{
    Fence fence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    fence.m_device              = m_handle;
    VK_CHECK( vkCreateFence( m_handle, &fenceInfo, nullptr, &fence.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name ) );

    return fence;
}

Semaphore Device::NewSemaphore( const std::string& name ) const
{
    Semaphore sem;
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem.m_device               = m_handle;
    VK_CHECK( vkCreateSemaphore( m_handle, &info, nullptr, &sem.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( sem, name ) );

    return sem;
}

AccelerationStructure Device::NewAccelerationStructure( AccelerationStructureType type, size_t size ) const
{
    BufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size             = size;
    bufferCreateInfo.bufferUsage |= BufferUsage::AS_STORAGE;
    bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    AccelerationStructure accelerationStructure;
    accelerationStructure.m_device = m_handle;
    accelerationStructure.m_type   = type;
    accelerationStructure.m_buffer = NewBuffer( bufferCreateInfo );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    accelerationStructureCreateInfo.buffer = accelerationStructure.m_buffer.GetHandle();
    accelerationStructureCreateInfo.size   = size;
    accelerationStructureCreateInfo.type   = PGToVulkanAccelerationStructureType( type );
    VK_CHECK( vkCreateAccelerationStructureKHR( m_handle, &accelerationStructureCreateInfo, nullptr, &accelerationStructure.m_handle ) );

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.m_handle;
    accelerationStructure.m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR( m_handle, &accelerationDeviceAddressInfo );

    return accelerationStructure;
}

Buffer Device::NewBuffer( const BufferCreateInfo& createInfo, const std::string& name ) const
{

    VkBufferCreateInfo vkBufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    vkBufInfo.usage       = PGToVulkanBufferType( createInfo.bufferUsage );
    vkBufInfo.size        = createInfo.size;
    vkBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage                   = createInfo.memoryUsage;
    vmaAllocInfo.flags                   = createInfo.flags;

    Buffer buffer;
    buffer.m_size        = createInfo.size;
    buffer.m_bufferUsage = createInfo.bufferUsage;
    buffer.m_memoryUsage = createInfo.memoryUsage;
    VmaAllocationInfo allocReturnInfo;
    vmaCreateBuffer( m_vmaAllocator, &vkBufInfo, &vmaAllocInfo, &buffer.m_handle, &buffer.m_allocation, &allocReturnInfo );
    buffer.m_persistent = ( createInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT ) && allocReturnInfo.pMappedData != nullptr;

    VkMemoryPropertyFlags vkMemProperties;
    vmaGetAllocationMemoryProperties( m_vmaAllocator, buffer.m_allocation, &vkMemProperties );
    buffer.m_coherent = ( vkMemProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    PG_ASSERT( !( buffer.m_persistent && !buffer.m_coherent ), "Persistently mapped buffers should be coherent" );

    buffer.m_mappedPtr = buffer.m_persistent ? allocReturnInfo.pMappedData : nullptr;

    return buffer;
}

// Buffer Device::NewBuffer( size_t length, void* data, BufferUsage type, MemoryType memoryType, const std::string& name ) const
//{
//     Buffer dstBuffer;
//     type |= BUFFER_TYPE_TRANSFER_SRC | BUFFER_TYPE_TRANSFER_DST;
//
//     if ( memoryType & MEMORY_TYPE_DEVICE_LOCAL )
//     {
//         Buffer stagingBuffer =
//             NewBuffer( length, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "staging" );
//         stagingBuffer.Map();
//         memcpy( stagingBuffer.MappedPtr(), data, length );
//         stagingBuffer.UnMap();
//
//         dstBuffer = NewBuffer( length, type, memoryType, name );
//         Copy( dstBuffer, stagingBuffer );
//         stagingBuffer.Free();
//     }
//     else if ( memoryType & MEMORY_TYPE_HOST_VISIBLE )
//     {
//         dstBuffer = NewBuffer( length, type, memoryType, name );
//         dstBuffer.Map();
//         memcpy( dstBuffer.MappedPtr(), data, length );
//         if ( ( memoryType & MEMORY_TYPE_HOST_COHERENT ) == 0 )
//         {
//             dstBuffer.FlushCpuWrites();
//         }
//         dstBuffer.UnMap();
//     }
//     else
//     {
//         PG_ASSERT( false, "Unknown MemoryType passed into NewBuffer. Not copying data into buffer" );
//     }
//
//     return dstBuffer;
// }

Texture Device::NewTexture( const TextureCreateInfo& desc, const std::string& name ) const
{
    bool isDepth                = PixelFormatHasDepthFormat( desc.format );
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType         = PGToVulkanImageType( desc.type );
    imageInfo.extent            = { desc.width, desc.height, desc.depth };
    imageInfo.mipLevels         = desc.mipLevels;
    imageInfo.arrayLayers       = desc.arrayLayers;
    imageInfo.format            = PGToVulkanPixelFormat( desc.format );
    imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage             = desc.usage;
    imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    if ( desc.arrayLayers == 6 )
    {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    Texture tex;
    tex.m_device  = m_handle;
    tex.m_desc    = desc;
    tex.m_sampler = GetSampler( desc.sampler );

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VK_CHECK( vmaCreateImage( m_vmaAllocator, &imageInfo, &allocInfo, &tex.m_image, &tex.m_allocation, nullptr ) );

    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    if ( isDepth )
    {
        features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, features ) );
    tex.m_imageView = CreateImageView(
        tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_IMAGE_NAME( tex.m_image, name ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( tex.m_imageView, name ) );

    // tex.m_bindlessArrayIndex = TextureManager::GetOpenSlot( &tex );

    return tex;
}

Texture Device::NewTextureFromBuffer( TextureCreateInfo& desc, void* data, const std::string& name ) const
{
    // desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Texture tex          = NewTexture( desc, name );
    // size_t imSize        = desc.TotalSizeInBytes();
    // Buffer stagingBuffer = NewBuffer( imSize, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    // stagingBuffer.Map();
    // memcpy( stagingBuffer.MappedPtr(), data, imSize );
    // stagingBuffer.UnMap();
    //
    // VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    // PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );
    //
    // TransitionImageLayoutImmediate( tex.GetImage(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     tex.m_desc.mipLevels, tex.m_desc.arrayLayers );
    // CopyBufferToImage( stagingBuffer, tex );
    // TransitionImageLayoutImmediate( tex.GetImage(), vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex.m_desc.mipLevels, tex.m_desc.arrayLayers );
    //
    // stagingBuffer.Free();
    //
    // return tex;
    return {};
}

Sampler Device::NewSampler( const SamplerDescriptor& desc ) const
{
    Sampler sampler;
    sampler.m_desc   = desc;
    sampler.m_device = m_handle;

    VkSamplerCreateInfo info     = {};
    info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter               = PGToVulkanFilterMode( desc.magFilter );
    info.minFilter               = PGToVulkanFilterMode( desc.minFilter );
    info.addressModeU            = PGToVulkanWrapMode( desc.wrapModeU );
    info.addressModeV            = PGToVulkanWrapMode( desc.wrapModeV );
    info.addressModeW            = PGToVulkanWrapMode( desc.wrapModeW );
    info.anisotropyEnable        = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy           = desc.maxAnisotropy;
    info.borderColor             = PGToVulkanBorderColor( desc.borderColor );
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable           = VK_FALSE;
    info.compareOp               = VK_COMPARE_OP_ALWAYS;
    info.mipmapMode              = PGToVulkanMipFilter( desc.mipFilter );
    info.mipLodBias              = 0.0f;
    info.minLod                  = 0.0f;
    info.maxLod                  = 100.0f;

    VK_CHECK( vkCreateSampler( m_handle, &info, nullptr, &sampler.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( desc.name, PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, desc.name ) );

    return sampler;
}

Pipeline Device::NewGraphicsPipeline( const PipelineDescriptor& desc, const std::string& name ) const
{
    PG_ASSERT( false, "todo" );
    return {};
}

Pipeline Device::NewComputePipeline( Shader* shader, const std::string& name ) const
{
    PG_ASSERT( false, "todo" );
    return {};
    // PG_ASSERT( shader && shader->shaderStage == ShaderStage::COMPUTE );
    //
    // Pipeline pipeline;
    // pipeline.m_device          = m_handle;
    // pipeline.m_desc.shaders[0] = shader;
    // pipeline.m_resourceLayout  = CombineShaderResourceLayouts( &shader, 1 );
    // pipeline.m_isCompute       = true;
    // auto& layouts              = pipeline.m_resourceLayout;
    // VkDescriptorSetLayout activeLayouts[PG_MAX_NUM_DESCRIPTOR_SETS];
    // uint32_t numActiveSets = 0;
    // ForEachBit( pipeline.m_resourceLayout.descriptorSetMask,
    //     [&]( uint32_t set )
    //     {
    //         RegisterDescriptorSetLayout( layouts.sets[set], layouts.bindingStages[set] );
    //         activeLayouts[numActiveSets++] = layouts.sets[set].GetHandle();
    //     } );
    //
    // VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    // pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutCreateInfo.setLayoutCount             = numActiveSets;
    // pipelineLayoutCreateInfo.pSetLayouts                = activeLayouts;
    // VK_CHECK( vkCreatePipelineLayout( m_handle, &pipelineLayoutCreateInfo, NULL, &pipeline.m_pipelineLayout ) );
    //
    // VkComputePipelineCreateInfo createInfo = {};
    // createInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    // createInfo.layout                      = pipeline.m_pipelineLayout;
    // createInfo.stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // createInfo.stage.stage                 = VK_SHADER_STAGE_COMPUTE_BIT;
    // createInfo.stage.module                = shader->handle;
    // createInfo.stage.pName                 = shader->entryPoint.c_str();
    //
    // VK_CHECK( vkCreateComputePipelines( m_handle, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline.m_pipeline ) );
    // return pipeline;
}

RenderPass Device::NewRenderPass( const RenderPassDescriptor& desc, const std::string& name ) const
{
    RenderPass pass;
    pass.desc     = desc;
    pass.m_device = m_handle;

    VkAttachmentDescription attachments[9];
    VkAttachmentReference attachmentRefs[9];
    uint8_t numAttachments = 0;
    for ( uint8_t i = 0; i < desc.numColorAttachments; ++i )
    {
        const ColorAttachmentDescriptor& attach = desc.colorAttachmentDescriptors[i];
        attachments[i].flags                    = 0;
        attachments[i].format                   = PGToVulkanPixelFormat( attach.format );
        attachments[i].samples                  = VK_SAMPLE_COUNT_1_BIT;
        attachments[i].loadOp                   = PGToVulkanLoadAction( attach.loadAction );
        attachments[i].storeOp                  = PGToVulkanStoreAction( attach.storeAction );
        attachments[i].stencilLoadOp            = PGToVulkanLoadAction( LoadAction::DONT_CARE );
        attachments[i].stencilStoreOp           = PGToVulkanStoreAction( StoreAction::DONT_CARE );
        attachments[i].initialLayout            = PGToVulkanImageLayout( attach.initialLayout );
        attachments[i].finalLayout              = PGToVulkanImageLayout( attach.finalLayout );

        attachmentRefs[i].attachment = i;
        attachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ++numAttachments;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = numAttachments;
    subpass.pColorAttachments    = attachmentRefs;

    VkAttachmentReference depthAttachmentRef = {};
    if ( desc.numDepthAttachments != 0 )
    {
        const DepthAttachmentDescriptor& attach = desc.depthAttachmentDescriptor;
        VkAttachmentDescription depthAttachment;
        depthAttachment.flags   = 0;
        depthAttachment.format  = PGToVulkanPixelFormat( attach.format );
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp  = PGToVulkanLoadAction( attach.loadAction );
        depthAttachment.storeOp = PGToVulkanStoreAction( attach.storeAction );
        PG_ASSERT( !PixelFormatHasStencil( attach.format ), "Stencil formats not implemented yet" );
        depthAttachment.stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
        depthAttachment.stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
        depthAttachment.initialLayout  = PGToVulkanImageLayout( attach.initialLayout );
        depthAttachment.finalLayout    = PGToVulkanImageLayout( attach.finalLayout );

        depthAttachmentRef.attachment = numAttachments;
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        attachments[numAttachments]     = depthAttachment;
        ++numAttachments;
    }

    VkSubpassDependency dependency = {};
    dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass          = 0;
    dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask       = 0;
    dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount        = numAttachments;
    renderPassInfo.pAttachments           = attachments;
    renderPassInfo.subpassCount           = 1;
    renderPassInfo.pSubpasses             = &subpass;
    renderPassInfo.dependencyCount        = 1;
    renderPassInfo.pDependencies          = &dependency;

    VK_CHECK( vkCreateRenderPass( m_handle, &renderPassInfo, nullptr, &pass.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name ) );

    return pass;
}

Framebuffer Device::NewFramebuffer( const std::vector<Texture*>& attachments, const RenderPass& renderPass, const std::string& name ) const
{
    PG_ASSERT( 0 < attachments.size() && attachments.size() <= 9 );
    VkImageView frameBufferAttachments[9];
    for ( size_t i = 0; i < attachments.size(); ++i )
    {
        PG_ASSERT( attachments[i] );
        frameBufferAttachments[i] = attachments[i]->GetView();
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass              = renderPass.GetHandle();
    framebufferInfo.attachmentCount         = static_cast<uint32_t>( attachments.size() );
    framebufferInfo.pAttachments            = frameBufferAttachments;
    framebufferInfo.width                   = attachments[0]->GetWidth();
    framebufferInfo.height                  = attachments[0]->GetHeight();
    framebufferInfo.layers                  = 1;

    return NewFramebuffer( framebufferInfo, name );
}

Framebuffer Device::NewFramebuffer( const VkFramebufferCreateInfo& info, const std::string& name ) const
{
    Framebuffer ret;
    ret.m_width  = info.width;
    ret.m_height = info.height;
    ret.m_device = m_handle;

    VK_CHECK( vkCreateFramebuffer( m_handle, &info, nullptr, &ret.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( ret, name ) );

    return ret;
}

void Device::Copy( Buffer dst, Buffer src ) const
{
    // CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time copy buffer -> buffer" );
    //
    // cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    // cmdBuf.CopyBuffer( dst, src );
    // cmdBuf.EndRecording();
    //
    // Submit( cmdBuf );
    // WaitForIdle(); // TODO: use barrier
    //
    // cmdBuf.Free();
}

void Device::CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips ) const
{
    // PG_ASSERT( tex.GetDepth() == 1 );
    // CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time CopyBufferToImage" );
    // cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    //
    // std::vector<VkBufferImageCopy> bufferCopyRegions;
    // uint32_t offset = 0;
    //
    // uint32_t numMips = tex.GetMipLevels();
    // if ( !copyAllMips )
    //{
    //     numMips = 1;
    // }
    //
    // uint32_t width  = tex.GetWidth();
    // uint32_t height = tex.GetHeight();
    // for ( uint32_t mip = 0; mip < numMips; ++mip )
    //{
    //     for ( uint32_t face = 0; face < tex.GetArrayLayers(); ++face )
    //     {
    //         VkBufferImageCopy region               = {};
    //         region.bufferOffset                    = offset;
    //         region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    //         region.imageSubresource.mipLevel       = mip;
    //         region.imageSubresource.baseArrayLayer = face;
    //         region.imageSubresource.layerCount     = 1;
    //         region.imageExtent.width               = width;
    //         region.imageExtent.height              = height;
    //         region.imageExtent.depth               = 1;
    //
    //         bufferCopyRegions.push_back( region );
    //         uint32_t size = NumBytesPerPixel( tex.GetPixelFormat() );
    //         if ( PixelFormatIsCompressed( tex.GetPixelFormat() ) )
    //         {
    //             uint32_t roundedWidth  = ( width + 3 ) & ~3u;
    //             uint32_t roundedHeight = ( height + 3 ) & ~3u;
    //             uint32_t numBlocksX    = roundedWidth / 4;
    //             uint32_t numBlocksY    = roundedHeight / 4;
    //             size *= numBlocksX * numBlocksY;
    //         }
    //         else
    //         {
    //             size *= width * height;
    //         }
    //         offset += size;
    //     }
    //     width  = Max( width >> 1, 1u );
    //     height = Max( height >> 1, 1u );
    // }
    //
    // vkCmdCopyBufferToImage( cmdBuf.GetHandle(), buffer.GetHandle(), tex.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //     static_cast<uint32_t>( bufferCopyRegions.size() ), bufferCopyRegions.data() );
    //
    // cmdBuf.EndRecording();
    // Submit( cmdBuf );
    // WaitForIdle();
    // cmdBuf.Free();
}

void Device::Present( const Swapchain& swapchain, const Semaphore& waitSemaphore ) const
{
    VkSwapchainKHR vkSwapchain   = swapchain.GetHandle();
    VkSemaphore vkSemaphore      = waitSemaphore.GetHandle(); // wait until the rendering work is all done
    uint32_t swapchainImageIndex = swapchain.GetCurrentImageIndex();

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pSwapchains      = &vkSwapchain;
    presentInfo.swapchainCount   = 1;
    presentInfo.pImageIndices    = &swapchainImageIndex;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &vkSemaphore;

    VK_CHECK( vkQueuePresentKHR( m_queue.queue, &presentInfo ) );
}

VkDevice Device::GetHandle() const { return m_handle; }
Queue Device::GetQueue() const { return m_queue; }
VmaAllocator Device::GetAllocator() const { return m_vmaAllocator; }
Device::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace PG::Gfx
