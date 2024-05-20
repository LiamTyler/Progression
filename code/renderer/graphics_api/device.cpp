#include "renderer/graphics_api/device.hpp"
#include "c_shared/defines.h"
#include "core/feature_defines.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_texture_manager.hpp"
#include "renderer/render_system.hpp"
#include "renderer/vulkan.hpp"
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
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( m_handle, "Primary" );

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
    allocatorCreateInfo.physicalDevice         = rg.physicalDevice;
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
    cmdBufInfo.commandBuffer = cmdBuf;

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

CommandPool Device::NewCommandPool( CommandPoolCreateFlags flags, std::string_view name ) const
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags                   = PGToVulkanCommandPoolCreateFlags( flags );
    poolInfo.queueFamilyIndex        = m_queue.familyIndex;

    CommandPool cmdPool;
    VK_CHECK( vkCreateCommandPool( m_handle, &poolInfo, nullptr, &cmdPool.m_handle ) );
    PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( cmdPool, name );

    return cmdPool;
}

Fence Device::NewFence( bool signaled, std::string_view name ) const
{
    Fence fence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    VK_CHECK( vkCreateFence( m_handle, &fenceInfo, nullptr, &fence.m_handle ) );
    PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name );

    return fence;
}

Semaphore Device::NewSemaphore( std::string_view name ) const
{
    Semaphore sem;
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK( vkCreateSemaphore( m_handle, &info, nullptr, &sem.m_handle ) );
    PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( sem, name );

    return sem;
}

AccelerationStructure Device::NewAccelerationStructure( AccelerationStructureType type, size_t size ) const
{
    BufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size             = size;
    bufferCreateInfo.bufferUsage |= BufferUsage::AS_STORAGE;
    bufferCreateInfo.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    AccelerationStructure accelerationStructure;
    accelerationStructure.m_type   = type;
    accelerationStructure.m_buffer = NewBuffer( bufferCreateInfo );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    accelerationStructureCreateInfo.buffer = accelerationStructure.m_buffer;
    accelerationStructureCreateInfo.size   = size;
    accelerationStructureCreateInfo.type   = PGToVulkanAccelerationStructureType( type );
    VK_CHECK( vkCreateAccelerationStructureKHR( m_handle, &accelerationStructureCreateInfo, nullptr, &accelerationStructure.m_handle ) );

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.m_handle;
    accelerationStructure.m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR( m_handle, &accelerationDeviceAddressInfo );

    return accelerationStructure;
}

Buffer Device::NewBuffer( const BufferCreateInfo& createInfo, std::string_view name ) const
{
    VkBufferCreateInfo vkBufInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    vkBufInfo.usage       = PGToVulkanBufferType( createInfo.bufferUsage );
    vkBufInfo.size        = createInfo.size;
    vkBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage                   = createInfo.memoryUsage;
    vmaAllocInfo.flags                   = createInfo.flags;

    Buffer buffer        = {};
    buffer.m_size        = createInfo.size;
    buffer.m_bufferUsage = createInfo.bufferUsage;
    buffer.m_memoryUsage = createInfo.memoryUsage;
    VmaAllocationInfo allocReturnInfo;
    vmaCreateBuffer( m_vmaAllocator, &vkBufInfo, &vmaAllocInfo, &buffer.m_handle, &buffer.m_allocation, &allocReturnInfo );
    PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer.m_handle, name );
    buffer.m_persistent = ( createInfo.flags & VMA_ALLOCATION_CREATE_MAPPED_BIT ) && allocReturnInfo.pMappedData != nullptr;

    VkMemoryPropertyFlags vkMemProperties;
    vmaGetAllocationMemoryProperties( m_vmaAllocator, buffer.m_allocation, &vkMemProperties );
    buffer.m_coherent = ( vkMemProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    PG_ASSERT( !( buffer.m_persistent && !buffer.m_coherent ), "Persistently mapped buffers should be coherent" );

    buffer.m_mappedPtr = buffer.m_persistent ? allocReturnInfo.pMappedData : nullptr;

#if USING( DEBUG_BUILD )
    if ( !name.empty() )
    {
        buffer.debugName = (char*)malloc( name.length() + 1 );
        strcpy( buffer.debugName, name.data() );
    }
#endif // #if USING( DEBUG_BUILD )

    if ( !createInfo.initalData )
        return buffer;

    if ( buffer.m_mappedPtr )
    {
        memcpy( buffer.m_mappedPtr, createInfo.initalData, createInfo.size );
    }
    else
    {
        Buffer staging = NewStagingBuffer( createInfo.size );
        memcpy( staging.GetMappedPtr(), createInfo.initalData, createInfo.size );
        rg.ImmediateSubmit( [&]( CommandBuffer& cmdBuf ) { cmdBuf.CopyBuffer( buffer, staging ); } );
        staging.Free();
    }

    return buffer;
}

Buffer Device::NewStagingBuffer( size_t size ) const
{
    BufferCreateInfo info{};
    info.size        = size;
    info.bufferUsage = BufferUsage::TRANSFER_SRC;
    info.memoryUsage = VMA_MEMORY_USAGE_AUTO;
    info.flags       = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    Buffer buf       = NewBuffer( info, "staging" );
    PG_ASSERT( buf.m_persistent && buf.m_coherent, "staging buffers assumed host visible + mapped, without any need for flush commands" );
    return buf;
}

Texture Device::NewTexture( const TextureCreateInfo& desc, std::string_view name ) const
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
    tex.m_info    = desc;
    tex.m_sampler = GetSampler( desc.sampler );

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VK_CHECK( vmaCreateImage( m_vmaAllocator, &imageInfo, &allocInfo, &tex.m_image, &tex.m_allocation, nullptr ) );

    TextureManager::Usage tmUsage = TextureManager::Usage::NONE;
    VkFormatFeatureFlags features = 0;
    if ( desc.usage & VK_IMAGE_USAGE_SAMPLED_BIT )
    {
        features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        tmUsage |= TextureManager::Usage::READ;
    }
    if ( desc.usage & VK_IMAGE_USAGE_STORAGE_BIT )
    {
        features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
        tmUsage |= TextureManager::Usage::WRITE;
    }
    if ( isDepth )
        features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, features ) );
    tex.m_imageView = CreateImageView(
        tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
    PG_DEBUG_MARKER_SET_IMAGE_NAME( tex.m_image, name );
    PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( tex.m_imageView, name );

    tex.m_bindlessArrayIndex = TextureManager::AddTexture( tex.m_imageView, tmUsage );

#if USING( DEBUG_BUILD )
    if ( !name.empty() )
    {
        tex.debugName = (char*)malloc( name.length() + 1 );
        strcpy( tex.debugName, name.data() );
    }
#endif // #if USING( DEBUG_BUILD )

    return tex;
}

Texture Device::NewTextureWithData( TextureCreateInfo& desc, void* data, std::string_view name ) const
{
    desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    Texture tex          = NewTexture( desc, name );
    size_t imSize        = desc.TotalSizeInBytes();
    Buffer stagingBuffer = NewStagingBuffer( imSize );
    memcpy( stagingBuffer.GetMappedPtr(), data, imSize );

    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );

    rg.ImmediateSubmit(
        [&]( CommandBuffer& cmdBuf )
        {
            cmdBuf.TransitionImageLayout( tex.GetImage(), ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST );

            std::vector<VkBufferImageCopy> bufferCopyRegions;
            uint32_t offset = 0;

            uint32_t numMips = tex.GetMipLevels();
            uint32_t width   = tex.GetWidth();
            uint32_t height  = tex.GetHeight();
            for ( uint32_t mip = 0; mip < numMips; ++mip )
            {
                for ( uint32_t face = 0; face < tex.GetArrayLayers(); ++face )
                {
                    VkBufferImageCopy region               = {};
                    region.bufferOffset                    = offset;
                    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel       = mip;
                    region.imageSubresource.baseArrayLayer = face;
                    region.imageSubresource.layerCount     = 1;
                    region.imageExtent.width               = width;
                    region.imageExtent.height              = height;
                    region.imageExtent.depth               = 1;

                    bufferCopyRegions.push_back( region );
                    uint32_t size = NumBytesPerPixel( tex.GetPixelFormat() );
                    if ( PixelFormatIsCompressed( tex.GetPixelFormat() ) )
                    {
                        uint32_t roundedWidth  = ( width + 3 ) & ~3u;
                        uint32_t roundedHeight = ( height + 3 ) & ~3u;
                        uint32_t numBlocksX    = roundedWidth / 4;
                        uint32_t numBlocksY    = roundedHeight / 4;
                        size *= numBlocksX * numBlocksY;
                    }
                    else
                    {
                        size *= width * height;
                    }
                    offset += size;
                }
                width  = Max( width >> 1, 1u );
                height = Max( height >> 1, 1u );
            }

            vkCmdCopyBufferToImage( cmdBuf, stagingBuffer, tex.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>( bufferCopyRegions.size() ), bufferCopyRegions.data() );

            cmdBuf.TransitionImageLayout( tex.GetImage(), ImageLayout::TRANSFER_DST, ImageLayout::SHADER_READ_ONLY );
        } );

    stagingBuffer.Free();

    return tex;
}

Sampler Device::NewSampler( const SamplerCreateInfo& desc ) const
{
    Sampler sampler;
    sampler.m_info = desc;

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
    PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, desc.name );

    return sampler;
}

Pipeline Device::NewGraphicsPipeline( const GraphicsPipelineCreateInfo& info, std::string_view name ) const
{
    Pipeline p       = {};
    p.m_stageFlags   = 0;
    p.m_bindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    p.m_pipelineType = PipelineType::GRAPHICS;

    int taskShaderIdx          = -1;
    int meshShaderIdx          = -1;
    int pushConstantLowOffset  = INT16_MAX;
    int pushConstantHighOffset = 0;

    PG_ASSERT( info.shaders.size() <= 3, "increase shaderStages size below" );
    VkPipelineShaderStageCreateInfo shaderStages[3];
    for ( int i = 0; i < (int)info.shaders.size(); ++i )
    {
        Shader* shader = info.shaders[i];
        PG_ASSERT( shader );
        ShaderStage stage      = shader->GetShaderStage();
        shaderStages[i]        = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        shaderStages[i].stage  = PGToVulkanShaderStage( stage );
        shaderStages[i].module = shader->GetHandle();
        shaderStages[i].pName  = "main";
        p.m_stageFlags |= PGToVulkanShaderStage( stage );

        const ShaderReflectData& rData = shader->GetReflectionData();
        pushConstantLowOffset          = Min<int>( pushConstantLowOffset, rData.pushConstantOffset );
        pushConstantHighOffset         = Max<int>( pushConstantHighOffset, rData.pushConstantOffset + rData.pushConstantSize );

        if ( stage == ShaderStage::TASK )
            taskShaderIdx = i;
        if ( stage == ShaderStage::MESH )
            meshShaderIdx = i;
    }
    PG_ASSERT( !( taskShaderIdx > -1 && meshShaderIdx == -1 ), "If using a task shader, there must be a corresponding mesh shader" );
    bool useMeshShading = meshShaderIdx != -1;
    if ( useMeshShading )
        p.m_workgroupSize = info.shaders[taskShaderIdx > -1 ? taskShaderIdx : meshShaderIdx]->GetReflectionData().workgroupSize;
    else
        p.m_workgroupSize = uvec3( 0 );

    int pushConstantSize       = Max( 0, pushConstantHighOffset - pushConstantLowOffset );
    VkPushConstantRange pRange = { p.m_stageFlags, (uint32_t)pushConstantLowOffset, (uint32_t)pushConstantSize };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount         = 1;
    pipelineLayoutCreateInfo.pSetLayouts            = &GetGlobalDescriptorSetLayout();
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantSize ? 1 : 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = pushConstantSize ? &pRange : nullptr;
    VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &p.m_pipelineLayout ) );

    VkDynamicState vkDynamicStates[2] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamicState.dynamicStateCount = ARRAY_COUNT( vkDynamicStates );
    dynamicState.pDynamicStates    = vkDynamicStates;

    VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = PGToVulkanPolygonMode( info.rasterizerInfo.polygonMode );
    rasterizer.lineWidth               = 1.0f; // to be higher than 1 needs the wideLines GPU feature
    rasterizer.cullMode                = PGToVulkanCullFace( info.rasterizerInfo.cullFace );
    rasterizer.frontFace               = PGToVulkanWindingOrder( info.rasterizerInfo.winding );
    rasterizer.depthBiasEnable         = info.rasterizerInfo.depthBiasEnable;

    VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    PG_ASSERT( info.colorAttachments.size() <= 8, "update array sizes below" );
    VkPipelineColorBlendAttachmentState colorBlendAttachment[8];
    VkFormat colorAttachmentFormats[8];
    for ( size_t i = 0; i < info.colorAttachments.size(); ++i )
    {
        const PipelineColorAttachmentInfo& attach = info.colorAttachments[i];
        colorBlendAttachment[i]                   = {};
        colorBlendAttachment[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment[i].blendEnable = attach.blendingEnabled;
        if ( attach.blendingEnabled )
        {
            colorBlendAttachment[i].srcColorBlendFactor = PGToVulkanBlendFactor( attach.srcColorBlendFactor );
            colorBlendAttachment[i].dstColorBlendFactor = PGToVulkanBlendFactor( attach.dstColorBlendFactor );
            colorBlendAttachment[i].srcAlphaBlendFactor = PGToVulkanBlendFactor( attach.srcAlphaBlendFactor );
            colorBlendAttachment[i].dstAlphaBlendFactor = PGToVulkanBlendFactor( attach.dstAlphaBlendFactor );
            colorBlendAttachment[i].colorBlendOp        = PGToVulkanBlendEquation( attach.colorBlendEquation );
            colorBlendAttachment[i].alphaBlendOp        = PGToVulkanBlendEquation( attach.alphaBlendEquation );
        }
        colorAttachmentFormats[i] = PGToVulkanPixelFormat( attach.format );
    }

    VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = (uint32_t)info.colorAttachments.size();
    colorBlending.pAttachments    = colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depthStencil.depthTestEnable       = info.depthInfo.depthTestEnabled;
    depthStencil.depthWriteEnable      = info.depthInfo.depthWriteEnabled;
    depthStencil.depthCompareOp        = PGToVulkanCompareFunction( info.depthInfo.compareFunc );
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipelineInfo.stageCount          = (uint32_t)info.shaders.size();
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = nullptr; // assuming pull-style vertex fetching always
    pipelineInfo.pInputAssemblyState = nullptr;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = p.m_pipelineLayout;

    VkFormat colorFmt = PGToVulkanPixelFormat( PixelFormat::R16_G16_B16_A16_FLOAT );
    VkPipelineRenderingCreateInfo dynRenderingCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    dynRenderingCreateInfo.colorAttachmentCount    = (uint32_t)info.colorAttachments.size();
    dynRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats;

    PixelFormat depthFmt = PixelFormat::INVALID;
    if ( info.depthInfo.format != PixelFormat::INVALID )
    {
        VkFormat fmt                                 = PGToVulkanPixelFormat( info.depthInfo.format );
        dynRenderingCreateInfo.depthAttachmentFormat = fmt;
        if ( PixelFormatHasStencil( info.depthInfo.format ) )
            dynRenderingCreateInfo.stencilAttachmentFormat = fmt;
    }

    pipelineInfo.pNext = &dynRenderingCreateInfo;

    VK_CHECK( vkCreateGraphicsPipelines( rg.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &p.m_pipeline ) );

    PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, name );
    PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, name );

    return p;
}

Pipeline Device::NewComputePipeline( Shader* shader, std::string_view name ) const
{
    Pipeline p        = {};
    p.m_stageFlags    = VK_SHADER_STAGE_COMPUTE_BIT;
    p.m_bindPoint     = VK_PIPELINE_BIND_POINT_COMPUTE;
    p.m_pipelineType  = PipelineType::COMPUTE;
    p.m_workgroupSize = shader->GetReflectionData().workgroupSize;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts    = &GetGlobalDescriptorSetLayout();
    VkPushConstantRange pRange              = {
        VK_SHADER_STAGE_COMPUTE_BIT, shader->GetReflectionData().pushConstantOffset, shader->GetReflectionData().pushConstantSize };
    pipelineLayoutCreateInfo.pushConstantRangeCount = pRange.size ? 1 : 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = pRange.size ? &pRange : nullptr;
    VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &p.m_pipelineLayout ) );

    VkPipelineShaderStageCreateInfo stageinfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = shader->GetHandle();
    stageinfo.pName  = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
#if USING( PG_DESCRIPTOR_BUFFER )
    computePipelineCreateInfo.flags |= VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
    computePipelineCreateInfo.layout = p.GetLayoutHandle();
    computePipelineCreateInfo.stage  = stageinfo;
    VK_CHECK( vkCreateComputePipelines( rg.device, nullptr, 1, &computePipelineCreateInfo, nullptr, &p.m_pipeline ) );
    PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( p.m_pipelineLayout, name );
    PG_DEBUG_MARKER_SET_PIPELINE_NAME( p.m_pipeline, name );

    return p;
}

void Device::CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips ) const { PG_ASSERT( false ); }

bool Device::Present( const Swapchain& swapchain, const Semaphore& waitSemaphore ) const
{
    VkSwapchainKHR vkSwapchain   = swapchain;
    VkSemaphore vkSemaphore      = waitSemaphore;
    uint32_t swapchainImageIndex = swapchain.GetCurrentImageIndex();

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pSwapchains      = &vkSwapchain;
    presentInfo.swapchainCount   = 1;
    presentInfo.pImageIndices    = &swapchainImageIndex;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &vkSemaphore;

    VkResult res      = vkQueuePresentKHR( m_queue.queue, &presentInfo );
    bool resizeNeeded = res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR;
    PG_ASSERT( res == VK_SUCCESS || resizeNeeded, "vkQueuePresentKHR failed with error %d", res );
    return !resizeNeeded;
}

VkDevice Device::GetHandle() const { return m_handle; }
Queue Device::GetQueue() const { return m_queue; }
VmaAllocator Device::GetAllocator() const { return m_vmaAllocator; }
Device::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace PG::Gfx
