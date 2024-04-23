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

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags                  = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = rg.physicalDevice.GetHandle();
    allocatorCreateInfo.device                 = m_handle;
    allocatorCreateInfo.instance               = rg.instance;
    VK_CHECK_RESULT( vmaCreateAllocator( &allocatorCreateInfo, &m_vmaAllocator ) );

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
    VK_CHECK_RESULT( vkQueueSubmit2( m_queue.queue, 1, &info, vkFence ) );
}

void Device::WaitForIdle() const { VK_CHECK_RESULT( vkQueueWaitIdle( m_queue.queue ) ); }

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

DescriptorPool Device::NewDescriptorPool(
    int numPoolSizes, VkDescriptorPoolSize* poolSizes, bool bindless, uint32_t maxSets, const std::string& name ) const
{
    DescriptorPool pool;
    pool.m_device = m_handle;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount              = numPoolSizes;
    poolInfo.pPoolSizes                 = poolSizes;
    poolInfo.maxSets                    = maxSets;
    poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    if ( bindless )
    {
        poolInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    }

    VK_CHECK_RESULT( vkCreateDescriptorPool( m_handle, &poolInfo, nullptr, &pool.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name ) );

    return pool;
}

bool Device::RegisterDescriptorSetLayout( DescriptorSetLayout& layout, const uint32_t stagesForBindings[PG_MAX_NUM_DESCRIPTOR_SETS] ) const
{
    layout.m_device                            = m_handle;
    VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // bindless info
    bool bindless                                            = false;
    VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    VkDescriptorBindingFlags bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                                        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

    for ( unsigned i = 0; i < PG_MAX_NUM_BINDINGS_PER_SET; ++i )
    {
        auto stages = stagesForBindings[i];
        if ( stages == 0 )
        {
            continue;
        }
        stages = VK_SHADER_STAGE_ALL;

        uint32_t arraySize = layout.arraySizes[i];
        if ( arraySize == UINT32_MAX )
        {
            bindless  = true;
            arraySize = PG_MAX_BINDLESS_TEXTURES;
        }

        unsigned types = 0;
        if ( layout.sampledImageMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, arraySize, stages } );
            ++types;
        }

        if ( layout.separateImageMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.storageImageMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.uniformBufferMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.storageBufferMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.uniformTexelBufferMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.storageTexelBufferMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.inputAttachmentMask & ( 1u << i ) )
        {
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, arraySize, stages, nullptr } );
            ++types;
        }

        if ( layout.samplerMask & ( 1u << i ) )
        {
            LOG_WARN( "No immutable sampler support yet" );
            bindings.push_back( { i, VK_DESCRIPTOR_TYPE_SAMPLER, arraySize, stages } );
            ++types;
        }

        PG_NO_WARN_UNUSED( types );
        PG_ASSERT( types <= 1, "Descriptor set aliasing!" );
    }

    if ( !bindings.empty() )
    {
        if ( bindless )
        {
            if ( bindings.size() != 1 )
            {
                LOG_ERR( "Using bindless, but binding count != 1" );
                return false;
            }
            else
            {
                extendedInfo.pNext         = nullptr;
                extendedInfo.bindingCount  = 1u;
                extendedInfo.pBindingFlags = &bindFlag;
                createInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
                createInfo.pNext = &extendedInfo;
            }
        }
        createInfo.bindingCount = static_cast<uint32_t>( bindings.size() );
        createInfo.pBindings    = bindings.data();
    }
    else
    {
        LOG_WARN( "Creating descriptor set with no bindings" );
    }

    if ( vkCreateDescriptorSetLayout( m_handle, &createInfo, nullptr, &layout.m_handle ) != VK_SUCCESS )
    {
        LOG_ERR( "Failed to create descriptor set layout" );
        layout.m_handle = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

void Device::UpdateDescriptorSets( uint32_t count, const VkWriteDescriptorSet* writes ) const
{
    vkUpdateDescriptorSets( m_handle, count, writes, 0, nullptr );
}

void Device::UpdateDescriptorSets( const std::vector<VkWriteDescriptorSet>& writeList ) const
{
    vkUpdateDescriptorSets( m_handle, (uint32_t)writeList.size(), writeList.data(), 0, nullptr );
}

void Device::UpdateDescriptorSet( const VkWriteDescriptorSet& writes ) const { vkUpdateDescriptorSets( m_handle, 1, &writes, 0, nullptr ); }

Fence Device::NewFence( bool signaled, const std::string& name ) const
{
    Fence fence;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags             = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    fence.m_device              = m_handle;
    VK_CHECK_RESULT( vkCreateFence( m_handle, &fenceInfo, nullptr, &fence.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name ) );

    return fence;
}

Semaphore Device::NewSemaphore( const std::string& name ) const
{
    Semaphore sem;
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem.m_device               = m_handle;
    VK_CHECK_RESULT( vkCreateSemaphore( m_handle, &info, nullptr, &sem.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( sem, name ) );

    return sem;
}

AccelerationStructure Device::NewAccelerationStructure( AccelerationStructureType type, size_t size ) const
{
    AccelerationStructure accelerationStructure;
    accelerationStructure.m_device = m_handle;
    accelerationStructure.m_type   = type;
    accelerationStructure.m_buffer =
        NewBuffer( size, BUFFER_TYPE_ACCELERATION_STRUCTURE_STORAGE | BUFFER_TYPE_DEVICE_ADDRESS, MEMORY_TYPE_DEVICE_LOCAL );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    accelerationStructureCreateInfo.buffer = accelerationStructure.m_buffer.GetHandle();
    accelerationStructureCreateInfo.size   = size;
    accelerationStructureCreateInfo.type   = PGToVulkanAccelerationStructureType( type );
    VK_CHECK_RESULT(
        vkCreateAccelerationStructureKHR( m_handle, &accelerationStructureCreateInfo, nullptr, &accelerationStructure.m_handle ) );

    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
    accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.m_handle;
    accelerationStructure.m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR( m_handle, &accelerationDeviceAddressInfo );

    return accelerationStructure;
}

Buffer Device::NewBuffer( size_t length, BufferType type, MemoryType memoryType, const std::string& name ) const
{
    type |= BUFFER_TYPE_TRANSFER_SRC | BUFFER_TYPE_TRANSFER_DST;
    Buffer buffer;
    buffer.m_device     = m_handle;
    buffer.m_type       = type;
    buffer.m_memoryType = memoryType;
    buffer.m_length     = length;

    VkBufferCreateInfo info{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.usage       = PGToVulkanBufferType( type );
    info.size        = length;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT( vkCreateBuffer( m_handle, &info, nullptr, &buffer.m_handle ) );

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( m_handle, buffer.m_handle, &memRequirements );

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    memoryAllocateFlagsInfo.flags = ( type & BUFFER_TYPE_DEVICE_ADDRESS ) ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR : 0;

    VkMemoryPropertyFlags memPropertyFlags = PGToVulkanMemoryType( memoryType );
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, memPropertyFlags );
    allocInfo.pNext           = &memoryAllocateFlagsInfo;
    VK_CHECK_RESULT( vkAllocateMemory( m_handle, &allocInfo, nullptr, &buffer.m_memory ) );

    VK_CHECK_RESULT( vkBindBufferMemory( m_handle, buffer.m_handle, buffer.m_memory, 0 ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name ) );

    if ( type & BUFFER_TYPE_DEVICE_ADDRESS )
    {
        VkBufferDeviceAddressInfo bufferAddressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        bufferAddressInfo.buffer = buffer.m_handle;
        buffer.m_deviceAddress   = vkGetBufferDeviceAddress( m_handle, &bufferAddressInfo );
    }

    return buffer;
}

Buffer Device::NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType, const std::string& name ) const
{
    Buffer dstBuffer;
    type |= BUFFER_TYPE_TRANSFER_SRC | BUFFER_TYPE_TRANSFER_DST;

    if ( memoryType & MEMORY_TYPE_DEVICE_LOCAL )
    {
        Buffer stagingBuffer =
            NewBuffer( length, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "staging" );
        stagingBuffer.Map();
        memcpy( stagingBuffer.MappedPtr(), data, length );
        stagingBuffer.UnMap();

        dstBuffer = NewBuffer( length, type, memoryType, name );
        Copy( dstBuffer, stagingBuffer );
        stagingBuffer.Free();
    }
    else if ( memoryType & MEMORY_TYPE_HOST_VISIBLE )
    {
        dstBuffer = NewBuffer( length, type, memoryType, name );
        dstBuffer.Map();
        memcpy( dstBuffer.MappedPtr(), data, length );
        if ( ( memoryType & MEMORY_TYPE_HOST_COHERENT ) == 0 )
        {
            dstBuffer.FlushCpuWrites();
        }
        dstBuffer.UnMap();
    }
    else
    {
        PG_ASSERT( false, "Unknown MemoryType passed into NewBuffer. Not copying data into buffer" );
    }

    return dstBuffer;
}

Texture Device::NewTexture( const TextureDescriptor& desc, const std::string& name ) const
{
    bool isDepth                = PixelFormatHasDepthFormat( desc.format );
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType         = PGToVulkanImageType( desc.type );
    imageInfo.extent.width      = desc.width;
    imageInfo.extent.height     = desc.height;
    imageInfo.extent.depth      = desc.depth;
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
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    Texture tex;
    tex.m_device  = m_handle;
    tex.m_desc    = desc;
    tex.m_sampler = nullptr;
    if ( !desc.sampler.empty() )
    {
        tex.m_sampler = GetSampler( desc.sampler );
    }

    VK_CHECK_RESULT( vkCreateImage( m_handle, &imageInfo, nullptr, &tex.m_image ) );
    // vkCreateImage( m_handle, &imageInfo, nullptr, &tex.m_image );

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( m_handle, tex.m_image, &memRequirements );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = memRequirements.size;
    allocInfo.memoryTypeIndex      = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    VK_CHECK_RESULT( vkAllocateMemory( m_handle, &allocInfo, nullptr, &tex.m_memory ) );
    VK_CHECK_RESULT( vkBindImageMemory( m_handle, tex.m_image, tex.m_memory, 0 ) );

    VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if ( isDepth )
    {
        features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, features ) );
    tex.m_imageView = CreateImageView(
        tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
    if ( desc.addToBindlessArray )
    {
        tex.m_bindlessArrayIndex = TextureManager::GetOpenSlot( &tex );
    }
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_IMAGE_NAME( tex, name ) );

    return tex;
}

Texture Device::NewTextureFromBuffer( TextureDescriptor& desc, void* data, const std::string& name ) const
{
    desc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    Texture tex          = NewTexture( desc, name );
    size_t imSize        = desc.TotalSizeInBytes();
    Buffer stagingBuffer = NewBuffer( imSize, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    stagingBuffer.Map();
    memcpy( stagingBuffer.MappedPtr(), data, imSize );
    stagingBuffer.UnMap();

    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );

    TransitionImageLayoutImmediate( tex.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        tex.m_desc.mipLevels, tex.m_desc.arrayLayers );
    CopyBufferToImage( stagingBuffer, tex );
    TransitionImageLayoutImmediate( tex.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tex.m_desc.mipLevels, tex.m_desc.arrayLayers );

    stagingBuffer.Free();

    return tex;
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

    VK_CHECK_RESULT( vkCreateSampler( m_handle, &info, nullptr, &sampler.m_handle ) );
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( desc.name, PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, desc.name ) );

    return sampler;
}

static PipelineResourceLayout CombineShaderResourceLayouts( Shader* const* shaders, int numShaders )
{
    PipelineResourceLayout combinedLayout = {};

    for ( int shaderIndex = 0; shaderIndex < numShaders; ++shaderIndex )
    {
        uint32_t stageMask                       = 1u << static_cast<uint32_t>( shaders[shaderIndex]->shaderStage );
        const ShaderResourceLayout& shaderLayout = shaders[shaderIndex]->resourceLayout;
        for ( unsigned set = 0; set < PG_MAX_NUM_DESCRIPTOR_SETS; set++ )
        {
            combinedLayout.sets[set].sampledImageMask |= shaderLayout.sets[set].sampledImageMask;
            combinedLayout.sets[set].separateImageMask |= shaderLayout.sets[set].separateImageMask;
            combinedLayout.sets[set].storageImageMask |= shaderLayout.sets[set].storageImageMask;
            combinedLayout.sets[set].uniformBufferMask |= shaderLayout.sets[set].uniformBufferMask;
            combinedLayout.sets[set].storageBufferMask |= shaderLayout.sets[set].storageBufferMask;
            combinedLayout.sets[set].uniformTexelBufferMask |= shaderLayout.sets[set].uniformTexelBufferMask;
            combinedLayout.sets[set].storageTexelBufferMask |= shaderLayout.sets[set].storageTexelBufferMask;
            combinedLayout.sets[set].samplerMask |= shaderLayout.sets[set].samplerMask;
            combinedLayout.sets[set].inputAttachmentMask |= shaderLayout.sets[set].inputAttachmentMask;

            uint32_t activeShaderBinds = shaderLayout.sets[set].sampledImageMask | shaderLayout.sets[set].separateImageMask |
                                         shaderLayout.sets[set].storageImageMask | shaderLayout.sets[set].uniformBufferMask |
                                         shaderLayout.sets[set].storageBufferMask | shaderLayout.sets[set].uniformTexelBufferMask |
                                         shaderLayout.sets[set].storageTexelBufferMask | shaderLayout.sets[set].samplerMask |
                                         shaderLayout.sets[set].inputAttachmentMask;

            if ( activeShaderBinds )
            {
                combinedLayout.setStages[set] |= stageMask;
                ForEachBit( activeShaderBinds,
                    [&]( uint32_t bit )
                    {
                        combinedLayout.bindingStages[set][bit] |= stageMask;

                        uint32_t& combinedSize = combinedLayout.sets[set].arraySizes[bit];
                        uint32_t shaderSize    = shaderLayout.sets[set].arraySizes[bit];
                        if ( combinedSize && combinedSize != shaderSize )
                        {
                            LOG_ERR( "Mismatch between array sizes in different shaders." );
                        }
                        else
                        {
                            combinedSize = shaderSize;
                        }
                    } );
            }
        }

        // Merge push constant ranges into one range. No obvious gain for actually doing multiple ranges.
        if ( shaderLayout.pushConstantSize != 0 )
        {
            combinedLayout.pushConstantRange.stageFlags |= stageMask;
            combinedLayout.pushConstantRange.size = std::max( combinedLayout.pushConstantRange.size, shaderLayout.pushConstantSize );
        }
    }

    combinedLayout.descriptorSetMask = 0;
    for ( unsigned set = 0; set < PG_MAX_NUM_DESCRIPTOR_SETS; ++set )
    {
        if ( combinedLayout.setStages[set] != 0 )
        {
            combinedLayout.descriptorSetMask |= 1u << set;

            for ( unsigned binding = 0; binding < PG_MAX_NUM_BINDINGS_PER_SET; ++binding )
            {
                auto& arraySize = combinedLayout.sets[set].arraySizes[binding];
                if ( arraySize == 0 )
                {
                    arraySize = 1;
                }
                else if ( arraySize != UINT32_MAX )
                {
                    for ( unsigned i = 1; i < arraySize; ++i )
                    {
                        if ( combinedLayout.bindingStages[set][binding + i] != 0 )
                        {
                            LOG_ERR(
                                "Detected binding aliasing for (%u, %u). Binding array with %u elements starting at (%u, %u) overlaps.",
                                set, binding + i, arraySize, set, binding );
                        }
                    }
                }
            }
        }
    }

    return combinedLayout;
}

Pipeline Device::NewGraphicsPipeline( const PipelineDescriptor& desc, const std::string& name ) const
{
    Pipeline p;
    p.m_desc   = desc;
    p.m_device = m_handle;

    VkPipelineShaderStageCreateInfo shaderStages[3];
    uint32_t numShaderStages = 0;
    for ( size_t i = 0; i < 3 && desc.shaders[i]; ++i )
    {
        ++numShaderStages;
        shaderStages[i]        = {};
        shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[i].stage  = PGToVulkanShaderStage( desc.shaders[i]->shaderStage );
        shaderStages[i].module = desc.shaders[i]->handle;
        shaderStages[i].pName  = desc.shaders[i]->entryPoint.c_str();
    }
    PG_ASSERT( numShaderStages > 0 );

    p.m_resourceLayout = CombineShaderResourceLayouts( &desc.shaders[0], numShaderStages );
    p.m_isCompute      = false;
    auto& layout       = p.m_resourceLayout;
    VkDescriptorSetLayout vkLayouts[PG_MAX_NUM_DESCRIPTOR_SETS];
    uint32_t numSetLayouts = 0;
    if ( p.m_resourceLayout.descriptorSetMask )
    {
        uint32_t highestUsedSet = 0;
        for ( unsigned set = 0; set < PG_MAX_NUM_DESCRIPTOR_SETS; ++set )
        {
            if ( p.m_resourceLayout.descriptorSetMask & ( 1u << set ) )
            {
                RegisterDescriptorSetLayout( layout.sets[set], layout.bindingStages[set] );
                vkLayouts[set] = layout.sets[set].GetHandle();
                highestUsedSet = set;
            }
            else
            {
                vkLayouts[set] = GetEmptyDescriptorSetLayout();
            }
        }
        numSetLayouts = highestUsedSet + 1;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount             = numSetLayouts;
    pipelineLayoutCreateInfo.pSetLayouts                = vkLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount     = 0;
    if ( layout.pushConstantRange.size > 0 )
    {
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges    = &layout.pushConstantRange;
    }
    VK_CHECK_RESULT( vkCreatePipelineLayout( m_handle, &pipelineLayoutCreateInfo, NULL, &p.m_pipelineLayout ) );

    uint32_t dynamicStateCount       = 2;
    VkDynamicState vkDnamicStates[3] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };
    if ( desc.rasterizerInfo.depthBiasEnable )
    {
        vkDnamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_DEPTH_BIAS;
    }
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount                = dynamicStateCount;
    dynamicState.pDynamicStates                   = vkDnamicStates;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1; // don't need to give actual viewport or scissor upfront, since they're dynamic
    viewportState.scissorCount                      = 1;

    // specify topology and if primitive restart is on
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = PGToVulkanPrimitiveType( desc.primitiveType );
    inputAssembly.primitiveRestartEnable                 = VK_FALSE;

    // rasterizer does rasterization, depth testing, face culling, and scissor test
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable                       = VK_FALSE;
    rasterizer.rasterizerDiscardEnable                = VK_FALSE;
    rasterizer.polygonMode                            = PGToVulkanPolygonMode( desc.rasterizerInfo.polygonMode );
    rasterizer.lineWidth                              = 1.0f; // to be higher than 1 needs the wideLines GPU feature
    rasterizer.cullMode                               = PGToVulkanCullFace( desc.rasterizerInfo.cullFace );
    rasterizer.frontFace                              = PGToVulkanWindingOrder( desc.rasterizerInfo.winding );
    rasterizer.depthBiasEnable                        = desc.rasterizerInfo.depthBiasEnable;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable                  = VK_FALSE;
    multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

    bool dynamicRendering = desc.renderPass == nullptr && desc.dynamicAttachmentInfo.HasInfo();
    uint8_t numColorAttachments =
        dynamicRendering ? desc.dynamicAttachmentInfo.numColorAttachments : desc.renderPass->desc.numColorAttachments;
    VkPipelineColorBlendAttachmentState colorBlendAttachment[8] = {};
    for ( uint8_t i = 0; i < numColorAttachments; ++i )
    {
        colorBlendAttachment[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable                       = VK_FALSE;
    colorBlending.logicOp                             = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount                     = numColorAttachments;
    colorBlending.pAttachments                        = colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable                       = desc.depthInfo.depthTestEnabled;
    depthStencil.depthWriteEnable                      = desc.depthInfo.depthWriteEnabled;
    depthStencil.depthCompareOp                        = PGToVulkanCompareFunction( desc.depthInfo.compareFunc );
    depthStencil.depthBoundsTestEnable                 = VK_FALSE;
    depthStencil.stencilTestEnable                     = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount                   = numShaderStages;
    pipelineInfo.pStages                      = shaderStages;
    pipelineInfo.pVertexInputState            = &p.m_desc.vertexDescriptor.GetHandle();
    pipelineInfo.pInputAssemblyState          = &inputAssembly;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterizer;
    pipelineInfo.pMultisampleState            = &multisampling;
    pipelineInfo.pDepthStencilState           = &depthStencil;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = &dynamicState;
    pipelineInfo.layout                       = p.m_pipelineLayout;
    pipelineInfo.renderPass                   = dynamicRendering ? nullptr : desc.renderPass->GetHandle();
    pipelineInfo.subpass                      = 0;
    pipelineInfo.basePipelineHandle           = VK_NULL_HANDLE;

    VkPipelineRenderingCreateInfoKHR dynRenderingCreateInfo;
    VkFormat dynRenderingFormats[MAX_COLOR_ATTACHMENTS];
    if ( dynamicRendering )
    {
        dynRenderingCreateInfo                         = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
        dynRenderingCreateInfo.colorAttachmentCount    = desc.dynamicAttachmentInfo.numColorAttachments;
        dynRenderingCreateInfo.pColorAttachmentFormats = &dynRenderingFormats[1];
        for ( uint8_t i = 0; i < numColorAttachments; ++i )
            dynRenderingFormats[1 + i] = PGToVulkanPixelFormat( desc.dynamicAttachmentInfo.colorAttachmentFormats[i] );

        if ( desc.dynamicAttachmentInfo.depthAttachmentFormat != PixelFormat::INVALID )
        {
            VkFormat fmt                                 = PGToVulkanPixelFormat( desc.dynamicAttachmentInfo.depthAttachmentFormat );
            dynRenderingCreateInfo.depthAttachmentFormat = fmt;
            if ( PixelFormatHasStencil( desc.dynamicAttachmentInfo.depthAttachmentFormat ) )
                dynRenderingCreateInfo.stencilAttachmentFormat = fmt;
        }

        pipelineInfo.pNext = &dynRenderingCreateInfo;
    }

    if ( vkCreateGraphicsPipelines( m_handle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &p.m_pipeline ) != VK_SUCCESS )
    {
        LOG_ERR( "Failed to create graphics pipeline '%s'", name.c_str() );
        vkDestroyPipelineLayout( m_handle, p.m_pipelineLayout, nullptr );
        p.m_pipeline = VK_NULL_HANDLE;
    }
    PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( name, PG_DEBUG_MARKER_SET_PIPELINE_NAME( p, name ) );

    return p;
}

Pipeline Device::NewComputePipeline( Shader* shader, const std::string& name ) const
{
    PG_ASSERT( shader && shader->shaderStage == ShaderStage::COMPUTE );

    Pipeline pipeline;
    pipeline.m_device          = m_handle;
    pipeline.m_desc.shaders[0] = shader;
    pipeline.m_resourceLayout  = CombineShaderResourceLayouts( &shader, 1 );
    pipeline.m_isCompute       = true;
    auto& layouts              = pipeline.m_resourceLayout;
    VkDescriptorSetLayout activeLayouts[PG_MAX_NUM_DESCRIPTOR_SETS];
    uint32_t numActiveSets = 0;
    ForEachBit( pipeline.m_resourceLayout.descriptorSetMask,
        [&]( uint32_t set )
        {
            RegisterDescriptorSetLayout( layouts.sets[set], layouts.bindingStages[set] );
            activeLayouts[numActiveSets++] = layouts.sets[set].GetHandle();
        } );

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount             = numActiveSets;
    pipelineLayoutCreateInfo.pSetLayouts                = activeLayouts;
    VK_CHECK_RESULT( vkCreatePipelineLayout( m_handle, &pipelineLayoutCreateInfo, NULL, &pipeline.m_pipelineLayout ) );

    VkComputePipelineCreateInfo createInfo = {};
    createInfo.sType                       = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.layout                      = pipeline.m_pipelineLayout;
    createInfo.stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage.stage                 = VK_SHADER_STAGE_COMPUTE_BIT;
    createInfo.stage.module                = shader->handle;
    createInfo.stage.pName                 = shader->entryPoint.c_str();

    VK_CHECK_RESULT( vkCreateComputePipelines( m_handle, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline.m_pipeline ) );
    return pipeline;
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

    VK_CHECK_RESULT( vkCreateRenderPass( m_handle, &renderPassInfo, nullptr, &pass.m_handle ) );
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

    VK_CHECK_RESULT( vkCreateFramebuffer( m_handle, &info, nullptr, &ret.m_handle ) );
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

    VK_CHECK_RESULT( vkQueuePresentKHR( m_queue.queue, &presentInfo ) );
}

VkDevice Device::GetHandle() const { return m_handle; }
Queue Device::GetQueue() const { return m_queue; }
VmaAllocator Device::GetAllocator() const { return m_vmaAllocator; }
Device::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace PG::Gfx
