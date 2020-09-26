#include "renderer/graphics_api/device.hpp"
#include "core/assert.hpp"
#include "core/platform_defines.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/vulkan.hpp"
#include "utils/logger.hpp"
#include <algorithm>
#include <cstring>
#include <set>


std::vector< const char* > VK_VALIDATION_LAYERS =
{
    "VK_LAYER_KHRONOS_validation"
};

std::vector< const char* > VK_DEVICE_EXTENSIONS =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace PG
{
namespace Gfx
{


    Device Device::CreateDefault()
    {
        Device device;
        const PhysicalDevice& pDev = r_globals.physicalDevice;
        std::set< uint32_t > uniqueQueueFamilies = { pDev.GetGraphicsQueueFamily(), pDev.GetPresentationQueueFamily(), pDev.GetComputeQueueFamily() };

        float queuePriority = 1.0f;
        std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
        for ( uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;
            queueCreateInfos.push_back( queueCreateInfo );
        }

        VkPhysicalDeviceFeatures features = pDev.GetFeatures();
        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount    = static_cast< uint32_t >( queueCreateInfos.size() );
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();
        createInfo.pEnabledFeatures        = &features;

        auto extensions = VK_DEVICE_EXTENSIONS;
        if ( pDev.ExtensionSupported( VK_EXT_DEBUG_MARKER_EXTENSION_NAME ) )
		{
			extensions.push_back( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
		}
        createInfo.enabledExtensionCount   = static_cast< uint32_t >( extensions.size() );
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Specify device specific validation layers (ignored after v1.1.123?)
#if !USING( SHIP_BUILD )
        createInfo.enabledLayerCount   = static_cast< uint32_t >( VK_VALIDATION_LAYERS.size() );
        createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
#else // #if !USING( SHIP_BUILD )
        createInfo.enabledLayerCount   = 0;
#endif // #else // #if !USING( SHIP_BUILD )

        if ( vkCreateDevice( pDev.GetHandle(), &createInfo, nullptr, &device.m_handle ) != VK_SUCCESS )
        {
            return {};
        }

        vkGetDeviceQueue( device.m_handle, pDev.GetGraphicsQueueFamily(),     0, &device.m_graphicsQueue );
        vkGetDeviceQueue( device.m_handle, pDev.GetPresentationQueueFamily(), 0, &device.m_presentQueue );
        vkGetDeviceQueue( device.m_handle, pDev.GetComputeQueueFamily(),      0, &device.m_computeQueue );

        return device;
    }


    void Device::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyDevice( m_handle, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }


    void Device::Submit( const CommandBuffer& cmdBuf ) const
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer vkCmdBuf      = cmdBuf.GetHandle();
        submitInfo.pCommandBuffers    = &vkCmdBuf;

        vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    }


    void Device::WaitForIdle() const
    {
        vkQueueWaitIdle( m_graphicsQueue );
    }


    CommandPool Device::NewCommandPool( CommandPoolCreateFlags flags, CommandPoolQueueFamily family, const std::string& name ) const
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = PGToVulkanCommandPoolCreateFlags( flags );
        poolInfo.queueFamilyIndex = r_globals.physicalDevice.GetGraphicsQueueFamily();
        if ( family == CommandPoolQueueFamily::COMPUTE )
        {
            poolInfo.queueFamilyIndex = r_globals.physicalDevice.GetComputeQueueFamily();
        }

        CommandPool cmdPool;
        cmdPool.m_device = m_handle;
        if ( vkCreateCommandPool( m_handle, &poolInfo, nullptr, &cmdPool.m_handle ) != VK_SUCCESS )
        {
            cmdPool.m_handle = VK_NULL_HANDLE;
        }
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( cmdPool, name ) );

        return cmdPool;
    }


    DescriptorPool Device::NewDescriptorPool( int numPoolSizes, VkDescriptorPoolSize* poolSizes, uint32_t maxSets, const std::string& name ) const
    {
        DescriptorPool pool;
        pool.m_device = m_handle;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = numPoolSizes;
        poolInfo.pPoolSizes    = poolSizes;
        poolInfo.maxSets       = maxSets;

        VkResult ret = vkCreateDescriptorPool( m_handle, &poolInfo, nullptr, &pool.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name ) );

        return pool;
    }


    std::vector< DescriptorSetLayout > Device::NewDescriptorSetLayouts( const std::vector< DescriptorSetLayoutData >& layoutData ) const
    {
        uint32_t maxSetNumber = 0;
        for ( const auto& layout : layoutData )
        {
            maxSetNumber = std::max( maxSetNumber, layout.setNumber );
        }

        std::vector< DescriptorSetLayout > layouts( maxSetNumber + 1 );
        for ( const auto& layout : layoutData )
        {
            VkResult ret = vkCreateDescriptorSetLayout( m_handle, &layout.createInfo, nullptr, &layouts[layout.setNumber].m_handle );
            PG_ASSERT( ret == VK_SUCCESS );
        }

        VkDescriptorSetLayoutCreateInfo emptyInfo = {};
        emptyInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        emptyInfo.bindingCount = 0;
        emptyInfo.pBindings    = nullptr;

        for ( uint32_t i = 0; i < maxSetNumber + 1; ++i )
        {
            layouts[i].m_device = m_handle;
            if ( layouts[i].m_handle == VK_NULL_HANDLE )
            {
                VkResult ret = vkCreateDescriptorSetLayout( m_handle, &emptyInfo, nullptr, &layouts[i].m_handle );
                PG_ASSERT( ret == VK_SUCCESS );
            }
        }

        return layouts; 
    }


    void Device::UpdateDescriptorSets( uint32_t count, const VkWriteDescriptorSet* writes ) const
    {
        vkUpdateDescriptorSets( m_handle, count, writes, 0, nullptr );
    }


    Fence Device::NewFence( bool signaled, const std::string& name ) const
    {
        Fence fence;
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        fence.m_device  = m_handle;
        VkResult ret    = vkCreateFence( m_handle, &fenceInfo, nullptr, &fence.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name ) );

        return fence;
    }


    Semaphore Device::NewSemaphore( const std::string& name ) const
    {
        Semaphore sem;
        VkSemaphoreCreateInfo info = {};
        info.sType    = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem.m_device  = m_handle;
        VkResult ret  = vkCreateSemaphore( m_handle, &info, nullptr, &sem.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( sem, name ) );

        return sem;
    }


    Buffer Device::NewBuffer( size_t length, BufferType type, MemoryType memoryType, const std::string& name ) const
    {
        Buffer buffer;
        buffer.m_device       = m_handle;
        buffer.m_type         = type;
        buffer.m_memoryType   = memoryType;
        buffer.m_length       = length;

        VkBufferCreateInfo info = {};
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.usage       = PGToVulkanBufferType( type );
        info.size        = length;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult ret     = vkCreateBuffer( m_handle, &info, nullptr, &buffer.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( m_handle, buffer.m_handle, &memRequirements );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize       = memRequirements.size;
        VkMemoryPropertyFlags flags    = PGToVulkanMemoryType( memoryType );
        allocInfo.memoryTypeIndex      = FindMemoryType( memRequirements.memoryTypeBits, flags );
        ret = vkAllocateMemory( m_handle, &allocInfo, nullptr, &buffer.m_memory );
        PG_ASSERT( ret == VK_SUCCESS );

        vkBindBufferMemory( m_handle, buffer.m_handle, buffer.m_memory, 0 );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name ) );

        return buffer;
    }


    Buffer Device::NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType, const std::string& name ) const
    {
        Buffer dstBuffer;

        if ( memoryType & MEMORY_TYPE_DEVICE_LOCAL )
        {
            Buffer stagingBuffer = NewBuffer( length, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            stagingBuffer.Map();
            memcpy( stagingBuffer.MappedPtr(), data, length );
            stagingBuffer.UnMap();

            dstBuffer = NewBuffer( length, type | BUFFER_TYPE_TRANSFER_DST, memoryType, name );
            Copy( dstBuffer, stagingBuffer );
            stagingBuffer.Free();
        }
        else if ( ( memoryType & MEMORY_TYPE_HOST_VISIBLE ) && ( memoryType & MEMORY_TYPE_HOST_COHERENT ) )
        {
            dstBuffer = NewBuffer( length, type, memoryType, name );
            dstBuffer.Map();
            memcpy( dstBuffer.MappedPtr(), data, length );
            dstBuffer.UnMap();
        }
        else
        {
            PG_ASSERT( false, "Unknown MemoryType passed into NewBuffer. Not copying data into buffer" );
        }

        return dstBuffer;
    }


    Texture Device::NewTexture( const ImageDescriptor& desc, bool managed, const std::string& name ) const
    {
        bool isDepth = PixelFormatHasDepthFormat( desc.format );
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = PGToVulkanImageType( desc.type );
        imageInfo.extent.width  = desc.width;
        imageInfo.extent.height = desc.height;
        imageInfo.extent.depth  = desc.depth;
        imageInfo.mipLevels     = desc.mipLevels;
        imageInfo.arrayLayers   = desc.arrayLayers;
        imageInfo.format        = PGToVulkanPixelFormat( desc.format );
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = desc.usage;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        if ( desc.arrayLayers == 6 )
        {
            imageInfo.flags         = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        Texture tex;
        tex.m_device  = m_handle;
        tex.m_desc    = desc;
        tex.m_sampler = nullptr; // RenderSystem::GetSampler( desc.sampler );
        //PG_ASSERT( tex.m_sampler, "Sampler '" + desc.sampler + "' not a valid sampler" );

        VkResult res = vkCreateImage( m_handle, &imageInfo, nullptr, &tex.m_image );
        PG_ASSERT( res == VK_SUCCESS );

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements( m_handle, tex.m_image, &memRequirements );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        res                       = vkAllocateMemory( m_handle, &allocInfo, nullptr, &tex.m_memory );
        PG_ASSERT( res == VK_SUCCESS);
        vkBindImageMemory( m_handle, tex.m_image, tex.m_memory, 0 );

        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if ( isDepth )
        {
            features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
        PG_ASSERT( FormatSupported( vkFormat, features ) );
        tex.m_imageView = CreateImageView( tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
        if ( managed )
        {
            tex.m_textureSlot = ~0; //TextureManager::GetOpenSlot( &tex );
        }
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_IMAGE_NAME( tex, name ) );

        return tex;
    }


    Texture Device::NewTextureFromBuffer( ImageDescriptor& desc, void* data, bool managed, const std::string& name ) const
    {
        return {};
        /*
        desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        Texture tex          = NewTexture( desc, managed, name );
        size_t imSize        = CalculateTotalTextureSize( desc );
        Buffer stagingBuffer = NewBuffer( imSize, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
        stagingBuffer.Map();
        memcpy( stagingBuffer.MappedPtr(), data, imSize );
        stagingBuffer.UnMap();

        VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
        PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );
        
        TransitionImageLayout( tex.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tex.m_desc.mipLevels, tex.m_desc.arrayLayers );
        CopyBufferToImage( stagingBuffer, tex );
        TransitionImageLayout( tex.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex.m_desc.mipLevels, tex.m_desc.arrayLayers );

        stagingBuffer.Free();

        return tex;
        */
    }


    Sampler Device::NewSampler( const SamplerDescriptor& desc ) const
    {
        Sampler sampler;
        sampler.m_desc   = desc;
        sampler.m_device = m_handle;

        VkSamplerCreateInfo info = {};
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

        VkResult res = vkCreateSampler( m_handle, &info, nullptr, &sampler.m_handle );
        PG_ASSERT( res == VK_SUCCESS );
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( desc.name, PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, desc.name ) );

        return sampler;
    }


    Pipeline Device::NewPipeline( const PipelineDescriptor& desc, const std::string& name ) const
    {
        Pipeline p;
        p.m_desc   = desc;
        p.m_device = m_handle;

        VkPipelineShaderStageCreateInfo shaderStages[3];
        uint32_t numShaderStages = 0;
        for ( size_t i = 0; i < 3 && desc.shaders[i]; ++i )
        {
            ++numShaderStages;
            // TODO: Reflection
            shaderStages[i] = {};
            shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage  = PGToVulkanShaderStage( desc.shaders[i]->shaderStage );
            shaderStages[i].module = desc.shaders[i]->handle;
            shaderStages[i].pName  = "main";
        }
        std::vector< VkDynamicState > dynamicStates = desc.dynamicStates;

        VkViewport viewport;
        viewport.x        = desc.viewport.x;
        viewport.y        = desc.viewport.y;
        viewport.width    = desc.viewport.width;
        viewport.height   = desc.viewport.height;
        viewport.minDepth = desc.viewport.minDepth;
        viewport.maxDepth = desc.viewport.maxDepth;

        VkRect2D scissor;
        scissor.offset = { desc.scissor.x, desc.scissor.y };
        scissor.extent = { (uint32_t) desc.scissor.width, (uint32_t) desc.scissor.height };

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;

        // specify topology and if primitive restart is on
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = PGToVulkanPrimitiveType( desc.primitiveType );
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // rasterizer does rasterization, depth testing, face culling, and scissor test 
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = PGToVulkanPolygonMode( desc.rasterizerInfo.polygonMode );
        rasterizer.lineWidth               = 1.0f; // to be higher than 1 needs the wideLines GPU feature
        rasterizer.cullMode                = PGToVulkanCullFace( desc.rasterizerInfo.cullFace );
        rasterizer.frontFace               = PGToVulkanWindingOrder( desc.rasterizerInfo.winding );
        rasterizer.depthBiasEnable         = desc.rasterizerInfo.depthBiasEnable;
        if ( rasterizer.depthBiasEnable )
        {
            dynamicStates.push_back( VK_DYNAMIC_STATE_DEPTH_BIAS );
        }

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast< uint32_t >( dynamicStates.size() );
        dynamicState.pDynamicStates    = dynamicStates.data();

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

        uint32_t numColorAttachments = 0;
        VkPipelineColorBlendAttachmentState colorBlendAttachment[8] = {};
        for ( uint32_t i = 0; i < 8; ++i )
        {
            if ( desc.renderPass->desc.colorAttachmentDescriptors[i].format == PixelFormat::INVALID )
            {
                break;
            }

            ++numColorAttachments;
            colorBlendAttachment[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment[i].blendEnable         = desc.colorAttachmentInfos[i].blendingEnabled;
            colorBlendAttachment[i].srcColorBlendFactor = PGToVulkanBlendFactor( desc.colorAttachmentInfos[i].srcColorBlendFactor );
            colorBlendAttachment[i].dstColorBlendFactor = PGToVulkanBlendFactor( desc.colorAttachmentInfos[i].dstColorBlendFactor );
            colorBlendAttachment[i].srcAlphaBlendFactor = PGToVulkanBlendFactor( desc.colorAttachmentInfos[i].srcAlphaBlendFactor );
            colorBlendAttachment[i].dstAlphaBlendFactor = PGToVulkanBlendFactor( desc.colorAttachmentInfos[i].dstAlphaBlendFactor );
            colorBlendAttachment[i].colorBlendOp        = PGToVulkanBlendEquation( desc.colorAttachmentInfos[i].colorBlendEquation );
            colorBlendAttachment[i].alphaBlendOp        = PGToVulkanBlendEquation( desc.colorAttachmentInfos[i].alphaBlendEquation );
        }

        // blending for all attachments / global settings
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount   = numColorAttachments;
        colorBlending.pAttachments      = colorBlendAttachment;

        VkDescriptorSetLayout layouts[8];
        PG_ASSERT( desc.descriptorSetLayouts.size() <= 8 );
        for ( size_t i = 0; i < desc.descriptorSetLayouts.size(); ++i )
        {
            layouts[i] = desc.descriptorSetLayouts[i].GetHandle();
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast< uint32_t >( desc.descriptorSetLayouts.size() );
        pipelineLayoutInfo.pSetLayouts    = layouts;

        // std::vector< VkPushConstantRange > pushConstants;
        // for ( int i = 0; i < static_cast< int >( shaderStages.size() ); ++i )
        // {
        //     for ( const auto& range : desc.shaders[i]->reflectInfo.pushConstants )
        //     {
        //         pushConstants.push_back( range );
        //     }
        // }
        // std::sort( pushConstants.begin(), pushConstants.end(), []( const auto& lhs, const auto& rhs ) { return lhs.offset < rhs.offset; } );
        // for ( size_t i = 1; i < pushConstants.size(); ++i )
        // {
        //     if ( pushConstants[i].offset == pushConstants[i-1].offset + pushConstants[i-1].size )
        //     {
        //         pushConstants[i-1].size += pushConstants[i].size;
        //         pushConstants[i-1].stageFlags |= pushConstants[i].stageFlags;
        //         pushConstants.erase( pushConstants.begin() + i );
        //         --i;
        //     }
        // }
        // pipelineLayoutInfo.pushConstantRangeCount = static_cast< uint32_t >( pushConstants.size() );
        // pipelineLayoutInfo.pPushConstantRanges    = pushConstants.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if ( vkCreatePipelineLayout( m_handle, &pipelineLayoutInfo, nullptr, &p.m_pipelineLayout ) != VK_SUCCESS )
        {
            return p;
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = desc.depthInfo.depthTestEnabled;
        depthStencil.depthWriteEnable      = desc.depthInfo.depthWriteEnabled;
        depthStencil.depthCompareOp        = PGToVulkanCompareFunction( desc.depthInfo.compareFunc );
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = numShaderStages;
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = &p.m_desc.vertexDescriptor.GetHandle();
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = p.m_pipelineLayout;
        pipelineInfo.renderPass          = desc.renderPass->GetHandle();
        pipelineInfo.renderPass          = desc.renderPass->GetHandle();
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

        if ( vkCreateGraphicsPipelines( m_handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &p.m_pipeline ) != VK_SUCCESS )
        {
            vkDestroyPipelineLayout( m_handle, p.m_pipelineLayout, nullptr );
            p.m_pipeline = VK_NULL_HANDLE;
        }
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_PIPELINE_NAME( p, name ) );

        return p;
    }


    RenderPass Device::NewRenderPass( const RenderPassDescriptor& desc, const std::string& name ) const
    {
        RenderPass pass;
        pass.desc     = desc;
        pass.m_device = m_handle;

        std::vector< VkAttachmentDescription > attachments;
        std::vector< VkAttachmentReference > attachmentRefs;

        for ( size_t i = 0; i < desc.colorAttachmentDescriptors.size(); ++i )
        {
            const auto& attach = desc.colorAttachmentDescriptors[i];
            if ( attach.format == PixelFormat::INVALID )
            {
                break;
            }

            attachments.push_back( {} );
            attachments[i].format         = PGToVulkanPixelFormat( attach.format );
            attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp         = PGToVulkanLoadAction( attach.loadAction );
            attachments[i].storeOp        = PGToVulkanStoreAction( attach.storeAction );
            attachments[i].stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
            attachments[i].stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
            attachments[i].initialLayout  = PGToVulkanImageLayout( attach.initialLayout );
            attachments[i].finalLayout    = PGToVulkanImageLayout( attach.finalLayout );

            attachmentRefs.push_back( {} );
            attachmentRefs[i].attachment = static_cast< uint32_t>( i );
            attachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format         = PGToVulkanPixelFormat( desc.depthAttachmentDescriptor.format );
        depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp         = PGToVulkanLoadAction( desc.depthAttachmentDescriptor.loadAction );
        depthAttachment.storeOp        = PGToVulkanStoreAction( desc.depthAttachmentDescriptor.storeAction );
        depthAttachment.stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
        depthAttachment.stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
        depthAttachment.initialLayout  = PGToVulkanImageLayout( desc.depthAttachmentDescriptor.initialLayout );
        depthAttachment.finalLayout    = PGToVulkanImageLayout( desc.depthAttachmentDescriptor.finalLayout );

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = static_cast< uint32_t >( attachments.size() );
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast< uint32_t >( attachmentRefs.size() );
        subpass.pColorAttachments    = attachmentRefs.data();
        if ( desc.depthAttachmentDescriptor.format != PixelFormat::INVALID )
        {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
            attachments.push_back( depthAttachment );
        }

        VkSubpassDependency dependency = {};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
        renderPassInfo.pAttachments    = attachments.data();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if ( vkCreateRenderPass( m_handle, &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
        {
            pass.m_handle = VK_NULL_HANDLE;
        }
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name ) );

        return pass;
    }


    Framebuffer Device::NewFramebuffer( const std::vector< Texture* >& attachments, const RenderPass& renderPass, const std::string& name ) const
    {
        PG_ASSERT( 0 < attachments.size() && attachments.size() <= 9 );
        VkImageView frameBufferAttachments[9];
        for ( size_t i = 0; i < attachments.size(); ++i )
        {
            PG_ASSERT( attachments[i] );
            frameBufferAttachments[i] = attachments[i]->GetView();
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass.GetHandle();
        framebufferInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
        framebufferInfo.pAttachments    = frameBufferAttachments;
        framebufferInfo.width           = attachments[0]->GetWidth();
        framebufferInfo.height          = attachments[0]->GetHeight();
        framebufferInfo.layers          = 1;

        return NewFramebuffer( framebufferInfo, name );
    }


    Framebuffer Device::NewFramebuffer( const VkFramebufferCreateInfo& info, const std::string& name ) const
    {
        Framebuffer ret;
        ret.m_width  = info.width;
        ret.m_height = info.height;
        ret.m_device = m_handle;
        if ( vkCreateFramebuffer( m_handle, &info, nullptr, &ret.m_handle ) != VK_SUCCESS )
        {
            ret.m_handle = VK_NULL_HANDLE;
        }
        PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( ret, name ) );

        return ret;
    }


    void Device::Copy( Buffer dst, Buffer src ) const
    {
        CommandBuffer cmdBuf = r_globals.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time copy buffer -> buffer" );

        cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
        cmdBuf.Copy( dst, src );
        cmdBuf.EndRecording();

        Submit( cmdBuf );
        WaitForIdle();

        cmdBuf.Free();
    }


    void Device::CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips ) const
    {
        CommandBuffer cmdBuf = r_globals.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time CopyBufferToImage" );
        cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

        std::vector< VkBufferImageCopy > bufferCopyRegions;
		uint32_t offset = 0;

        uint32_t numMips = tex.GetMipLevels();
        if ( !copyAllMips )
        {
            numMips = 1;
        }
        for ( uint32_t face = 0; face < tex.GetArrayLayers(); ++face )
        {
            uint32_t width  = tex.GetWidth();
            uint32_t height = tex.GetHeight();
            for ( uint32_t mip = 0; mip < numMips; ++mip )
            {
                VkBufferImageCopy region               = {};
                region.bufferOffset                    = offset;
                region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel       = mip;
                region.imageSubresource.baseArrayLayer = face;
                region.imageSubresource.layerCount     = 1;
                region.imageExtent.width               = width;
                region.imageExtent.height              = height;
                region.imageExtent.depth               = tex.GetDepth();

                bufferCopyRegions.push_back( region );
                uint32_t size = NumBytesPerPixel( tex.GetPixelFormat() );
                if ( PixelFormatIsCompressed( tex.GetPixelFormat() ) )
                {
                    PG_ASSERT( false );
                    // uint32_t roundedWidth  = ( width  + 3 ) & ~3;
                    // uint32_t roundedHeight = ( height + 3 ) & ~3;
                    // uint32_t numBlocksX    = roundedWidth  / 4;
                    // uint32_t numBlocksY    = roundedHeight / 4;
                    // size                  *= numBlocksX * numBlocksY;
                }
                else
                {
                    size *= width * height;
                }
                offset += size;

                width  = std::max( width  >> 1, 1u );
                height = std::max( height >> 1, 1u );
            }
        }

        vkCmdCopyBufferToImage(
            cmdBuf.GetHandle(),
            buffer.GetHandle(),
            tex.GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast< uint32_t >( bufferCopyRegions.size() ),
            bufferCopyRegions.data()
        );

        cmdBuf.EndRecording();
        r_globals.device.Submit( cmdBuf );
        r_globals.device.WaitForIdle();
        cmdBuf.Free();
    }


    VkDevice Device::GetHandle() const
    {
        return m_handle;
    }


    VkQueue Device::GraphicsQueue() const
    {
        return m_graphicsQueue;
    }


    VkQueue Device::PresentQueue() const
    {
        return m_presentQueue;
    }


    Device::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    void Device::SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const
    {
        PG_ASSERT( 0 <= numBuffers && numBuffers <= 5 );
        VkCommandBuffer vkCmdBufs[5];
        for ( int i = 0; i < numBuffers; ++i )
        {
            vkCmdBufs[i] = cmdBufs[i].GetHandle();
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = { r_globals.presentCompleteSemaphore.GetHandle() };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = numBuffers;
        submitInfo.pCommandBuffers        = vkCmdBufs;

        VkSemaphore signalSemaphores[]    = { r_globals.renderCompleteSemaphore.GetHandle() };
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = signalSemaphores;

        VkResult ret = vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        PG_ASSERT( ret == VK_SUCCESS );
    }


    void Device::SubmitComputeCommand( const CommandBuffer& cmdBuf ) const
    {
        VkCommandBuffer buff = cmdBuf.GetHandle();
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &buff;

        VkSemaphore signalSemaphores[]    = { r_globals.renderCompleteSemaphore.GetHandle() };
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = signalSemaphores;

        VkResult ret = vkQueueSubmit( m_computeQueue, 1, &submitInfo, r_globals.computeFence.GetHandle() );
        PG_ASSERT( ret == VK_SUCCESS );
    }


    void Device::SubmitFrame( uint32_t imageIndex ) const
    {
        VkSemaphore signalSemaphores[] = { r_globals.renderCompleteSemaphore.GetHandle() };
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = { r_globals.swapchain.GetHandle() };
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;

        vkQueuePresentKHR( m_presentQueue, &presentInfo );
        
        vkDeviceWaitIdle( r_globals.device.GetHandle() );
    }


} // namespace Gfx
} // namespace PG
