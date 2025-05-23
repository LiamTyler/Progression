#include "renderer/graphics_api/device.hpp"
#include "c_shared/defines.h"
#include "core/feature_defines.hpp"
#include "core/pixel_formats.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_bindless_manager.hpp"
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
    PGP_ZONE_SCOPEDN( "Device::Create" );
    m_handle = vkbDevice.device;
    PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( m_handle, "Primary" );

    // TODO: actually ensure this queue supports all the operations we need
    auto queueRet = vkbDevice.get_queue( vkb::QueueType::graphics );
    if ( !queueRet )
    {
        LOG_ERR( "Could not get graphics queue. Error: %s", queueRet.error().message().c_str() );
        return false;
    }
    Queue& mainQ      = m_queues[Underlying( QueueType::GRAPHICS )];
    mainQ.queue       = queueRet.value();
    mainQ.familyIndex = vkbDevice.get_queue_index( vkb::QueueType::graphics ).value();
    mainQ.queueIndex  = 0;
    PG_DEBUG_MARKER_SET_QUEUE_NAME( mainQ.queue, "Primary GCT" );

    queueRet = vkbDevice.get_dedicated_queue( vkb::QueueType::transfer );
    if ( !queueRet )
    {
        LOG_ERR( "Could not get dedicated transfer queue. Error: %s", queueRet.error().message().c_str() );
        return false;
    }
    Queue& transferQ      = m_queues[Underlying( QueueType::TRANSFER )];
    transferQ.queue       = queueRet.value();
    transferQ.familyIndex = vkbDevice.get_dedicated_queue_index( vkb::QueueType::transfer ).value();
    transferQ.queueIndex  = 0;
    PG_DEBUG_MARKER_SET_QUEUE_NAME( transferQ.queue, "Dedicated Transfer" );

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice         = rg.physicalDevice;
    allocatorCreateInfo.device                 = m_handle;
    allocatorCreateInfo.instance               = rg.instance;
    VK_CHECK( vmaCreateAllocator( &allocatorCreateInfo, &m_vmaAllocator ) );

    m_uploadBufferManager.Init();

    return true;
}

void Device::Free()
{
    m_uploadBufferManager.Free();
    vmaDestroyAllocator( m_vmaAllocator );
    if ( m_handle != VK_NULL_HANDLE )
    {
        vkDestroyDevice( m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }
}

void Device::Submit( QueueType queue, const CommandBuffer& cmdBuf, const VkSemaphoreSubmitInfo* waitSemaphoreInfo,
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
    VK_CHECK( vkQueueSubmit2( GetQueue( queue ), 1, &info, vkFence ) );
}

void Device::WaitForIdle() const { VK_CHECK( vkDeviceWaitIdle( m_handle ) ); }

CommandPool Device::NewCommandPool( const CommandPoolCreateInfo& info, std::string_view name ) const
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags                   = PGToVulkanCommandPoolCreateFlags( info.flags );
    poolInfo.queueFamilyIndex        = GetQueue( info.queueType ).familyIndex;

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
    bufferCreateInfo.bufferUsage |= BufferUsage::ACCEL_STORAGE;
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
    vkBufInfo.usage       = PGToVulkanBufferType( createInfo.bufferUsage | BufferUsage::DEVICE_ADDRESS );
    vkBufInfo.size        = createInfo.size;
    vkBufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage                   = createInfo.memoryUsage;
    vmaAllocInfo.flags                   = createInfo.flags;

    Buffer buffer        = {};
    buffer.m_size        = createInfo.size;
    buffer.m_bufferUsage = createInfo.bufferUsage | BufferUsage::DEVICE_ADDRESS;
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

    VkBufferDeviceAddressInfo bdaInfo{};
    bdaInfo.sType          = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bdaInfo.buffer         = buffer.m_handle;
    buffer.m_deviceAddress = vkGetBufferDeviceAddress( m_handle, &bdaInfo );

    if ( createInfo.addToBindlessArray )
        buffer.m_bindlessIndex = BindlessManager::AddBuffer( &buffer );
    else
        buffer.m_bindlessIndex = PG_INVALID_BUFFER_INDEX;

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

Buffer Device::NewStagingBuffer( u64 size ) const
{
    BufferCreateInfo info{};
    info.size               = size;
    info.bufferUsage        = BufferUsage::TRANSFER_SRC;
    info.memoryUsage        = VMA_MEMORY_USAGE_AUTO;
    info.flags              = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    info.addToBindlessArray = false;
    Buffer buf              = NewBuffer( info, "staging" );
    PG_ASSERT( buf.m_persistent && buf.m_coherent, "staging buffers assumed host visible + mapped, without any need for flush commands" );
    return buf;
}

Texture Device::NewTexture( const TextureCreateInfo& desc, std::string_view name ) const
{
    Texture tex;
    NewTextureInPlace( tex, desc, name );
    return tex;
}

void Device::NewTextureInPlace( Texture& tex, const TextureCreateInfo& desc, std::string_view name ) const
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

    tex.m_info    = desc;
    tex.m_sampler = GetSampler( desc.sampler );

#if USING( DEBUG_BUILD )
    if ( !name.empty() )
    {
        tex.debugName = (char*)malloc( name.length() + 1 );
        strcpy( tex.debugName, name.data() );
    }
#endif // #if USING( DEBUG_BUILD )

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VK_CHECK( vmaCreateImage( m_vmaAllocator, &imageInfo, &allocInfo, &tex.m_image, &tex.m_allocation, nullptr ) );

    VkFormatFeatureFlags features = 0;
    if ( desc.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT )
        features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if ( desc.usage & VK_IMAGE_USAGE_SAMPLED_BIT )
        features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    if ( desc.usage & VK_IMAGE_USAGE_STORAGE_BIT )
        features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    if ( isDepth )
        features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
    PG_ASSERT( FormatSupported( vkFormat, features ) );
    tex.m_imageView = CreateImageView(
        tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT, desc.mipLevels, desc.arrayLayers );
    PG_DEBUG_MARKER_SET_IMAGE_NAME( tex.m_image, name );
    PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( tex.m_imageView, name );

    tex.m_bindlessIndex = BindlessManager::AddTexture( &tex );
}

void Device::AddUploadRequest( const Buffer& buffer, const void* data, u64 size, u64 offset )
{
    if ( size == 0 )
        return;

    UploadRequest req;
    req.type   = UploadCommandType::BUFFER_UPLOAD;
    req.buffer = buffer;
    req.size   = static_cast<u32>( size );
    req.offset = offset;

    m_uploadBufferManager.AddRequest( req, (const char*)data );
}

void Device::AddUploadRequest( const Texture& tex, const void* data )
{
    PGP_ZONE_SCOPEDN( "AddUploadRequest_Texture" );
    UploadRequest tbReq( UploadCommandType::LAYOUT_TRANSITION_PRE );
    tbReq.image = tex.GetImage();
    m_uploadBufferManager.AddRequest( tbReq, nullptr );

    UploadRequest req( UploadCommandType::TEXTURE_UPLOAD );
    req.image       = tex.GetImage();
    u32 numMips     = tex.GetMipLevels();
    u32 width       = tex.GetWidth();
    u32 height      = tex.GetHeight();
    u64 localOffset = 0;

    for ( u32 mip = 0; mip < numMips; ++mip )
    {
        for ( u32 face = 0; face < tex.GetArrayLayers(); ++face )
        {
            req.texReq.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            req.texReq.imageSubresource.mipLevel       = mip;
            req.texReq.imageSubresource.baseArrayLayer = face;
            req.texReq.imageSubresource.layerCount     = 1;
            req.texReq.imageExtent.width               = width;
            req.texReq.imageExtent.height              = height;
            req.texReq.imageExtent.depth               = 1;

            u32 size = NumBytesPerPixel( tex.GetPixelFormat() );
            if ( PixelFormatIsCompressed( tex.GetPixelFormat() ) )
            {
                u32 roundedWidth  = ( width + 3 ) & ~3u;
                u32 roundedHeight = ( height + 3 ) & ~3u;
                u32 numBlocksX    = roundedWidth / 4;
                u32 numBlocksY    = roundedHeight / 4;
                size *= numBlocksX * numBlocksY;
            }
            else
            {
                size *= width * height;
            }

            req.size = size;
            m_uploadBufferManager.AddRequest( req, (const char*)data + localOffset );
            localOffset += size;
        }
        width  = Max( width >> 1, 1u );
        height = Max( height >> 1, 1u );
    }

    UploadRequest taReq( UploadCommandType::LAYOUT_TRANSITION_POST );
    taReq.image = tex.GetImage();
    m_uploadBufferManager.AddRequest( taReq, nullptr );
}

void Device::FlushUploadRequests()
{
    PGP_ZONE_SCOPEDN( "FlushUploadRequests" );
    m_uploadBufferManager.FlushAll();
}

void Device::AcquirePendingTransfers()
{
    std::vector<VkBufferMemoryBarrier2>& pendingBuffers = m_uploadBufferManager.pendingBufferBarriers;
    std::vector<VkImageMemoryBarrier2>& pendingImages   = m_uploadBufferManager.pendingImageBarriers;
    if ( pendingBuffers.empty() && pendingImages.empty() )
        return;

    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.bufferMemoryBarrierCount = (u32)pendingBuffers.size();
    depInfo.pBufferMemoryBarriers    = pendingBuffers.data();
    depInfo.imageMemoryBarrierCount  = (u32)pendingImages.size();
    depInfo.pImageMemoryBarriers     = pendingImages.data();

    vkCmdPipelineBarrier2( rg.GetFrameData().primaryCmdBuffer, &depInfo );

    pendingBuffers.clear();
    pendingImages.clear();
}

Sampler Device::NewSampler( const SamplerCreateInfo& desc ) const
{
    Sampler sampler;
    sampler.m_info = desc;

    VkSamplerCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter           = PGToVulkanFilterMode( desc.magFilter );
    info.minFilter           = PGToVulkanFilterMode( desc.minFilter );
    info.addressModeU        = PGToVulkanWrapMode( desc.wrapModeU );
    info.addressModeV        = PGToVulkanWrapMode( desc.wrapModeV );
    info.addressModeW        = PGToVulkanWrapMode( desc.wrapModeW );
    info.anisotropyEnable    = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy       = desc.maxAnisotropy;
    info.borderColor         = PGToVulkanBorderColor( desc.borderColor );
    info.compareEnable       = VK_FALSE;
    info.compareOp           = VK_COMPARE_OP_NEVER;
    info.mipmapMode          = PGToVulkanMipFilter( desc.mipFilter );
    info.mipLodBias          = 0.0f;
    info.minLod              = 0.0f;
    info.maxLod              = VK_LOD_CLAMP_NONE;

    VK_CHECK( vkCreateSampler( m_handle, &info, nullptr, &sampler.m_handle ) );
    PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, desc.name );

    return sampler;
}

void Device::CopyBufferToImage( const Buffer& buffer, const Texture& tex, bool copyAllMips ) const { PG_ASSERT( false ); }

bool Device::Present( const Swapchain& swapchain, const Semaphore& waitSemaphore ) const
{
    VkSwapchainKHR vkSwapchain = swapchain;
    VkSemaphore vkSemaphore    = waitSemaphore;
    u32 swapchainImageIndex    = swapchain.GetCurrentImageIndex();

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.pSwapchains      = &vkSwapchain;
    presentInfo.swapchainCount   = 1;
    presentInfo.pImageIndices    = &swapchainImageIndex;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &vkSemaphore;

    VkResult res      = vkQueuePresentKHR( GetQueue( QueueType::GRAPHICS ), &presentInfo );
    bool resizeNeeded = res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR;
    PG_ASSERT( res == VK_SUCCESS || resizeNeeded, "vkQueuePresentKHR failed with error %d", res );
    return !resizeNeeded;
}

VkDevice Device::GetHandle() const { return m_handle; }
Queue Device::GetQueue( QueueType type ) const { return m_queues[Underlying( type )]; }
VmaAllocator Device::GetAllocator() const { return m_vmaAllocator; }
Device::operator bool() const { return m_handle != VK_NULL_HANDLE; }

} // namespace PG::Gfx
