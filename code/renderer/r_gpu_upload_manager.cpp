#include "r_gpu_upload_manager.hpp"
#include "r_globals.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"

static constexpr u32 BUFF_SIZE = 64 * 1024 * 1024;

#define TRANSFER_QUEUE_UPLOADS IN_USE

namespace PG::Gfx
{

void UploadManager::Init()
{
    CommandPoolCreateInfo cmdPoolCI;
    cmdPoolCI.flags     = COMMAND_POOL_RESET_COMMAND_BUFFER;
    cmdPoolCI.queueType = USING( TRANSFER_QUEUE_UPLOADS ) ? QueueType::TRANSFER : QueueType::GRAPHICS;
    cmdPool             = rg.device.NewCommandPool( cmdPoolCI, "UploadManager" );
    pendingBufferBarriers.reserve( 1024 );
    pendingImageBarriers.reserve( 1024 );

    for ( u32 i = 0; i < NUM_BUFFERS; ++i )
    {
        std::string iStr = std::to_string( i );
        BufferCreateInfo bCI{};
        bCI.size               = BUFF_SIZE;
        bCI.bufferUsage        = BufferUsage::TRANSFER_SRC;
        bCI.memoryUsage        = VMA_MEMORY_USAGE_AUTO;
        bCI.flags              = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        bCI.addToBindlessArray = false;
        stagingBuffers[i]      = rg.device.NewBuffer( bCI, "UploadManager " + iStr );
        currentSizes[i]        = 0;
        fences[i]              = rg.device.NewFence( true, "UploadManager " + iStr );
        cmdBufs[i]             = cmdPool.NewCommandBuffer( "UploadManager " + iStr );
        requests[i].reserve( 1024 );
    }
    currentBufferIdx = 0;
}

void UploadManager::Free()
{
    for ( u32 i = 0; i < UploadManager::NUM_BUFFERS; ++i )
    {
        stagingBuffers[i].Free();
        fences[i].Free();
    }
    cmdPool.Free();
}

void UploadManager::AddRequest( const UploadRequest& req, const char* data )
{
    PG_ASSERT( req.size <= BUFF_SIZE || req.type == UploadCommandType::BUFFER_UPLOAD,
        "Currently only buffer uploads can be broken into sub-pieces" );
    if ( req.size > BUFF_SIZE )
    {
        u32 numPieces          = ( req.size + BUFF_SIZE - 1 ) / BUFF_SIZE;
        UploadRequest pieceReq = req;
        u64 subOffset          = 0;
        for ( u32 i = 0; i < numPieces; ++i )
        {
            pieceReq.offset = req.offset + subOffset;
            pieceReq.size   = Min( BUFF_SIZE, req.size - i * BUFF_SIZE );
            AddRequest( pieceReq, data + subOffset );
            subOffset += BUFF_SIZE;
        }
        return;
    }

    if ( currentSizes[currentBufferIdx] + req.size > BUFF_SIZE )
    {
        SwapBuffers();
    }
    if ( req.size )
    {
        char* dst = stagingBuffers[currentBufferIdx].GetMappedPtr() + currentSizes[currentBufferIdx];
        memcpy( dst, data, req.size );
        currentSizes[currentBufferIdx] += req.size;
    }

    if ( req.type == UploadCommandType::BUFFER_UPLOAD && !requests[currentBufferIdx].empty() )
    {
        UploadRequest& prevReq = requests[currentBufferIdx].back();
        if ( prevReq.type == UploadCommandType::BUFFER_UPLOAD && prevReq.buffer == req.buffer &&
             ( prevReq.offset + prevReq.size ) == req.offset )
        {
            prevReq.size += req.size;
            return;
        }
    }
    requests[currentBufferIdx].push_back( req );
}

void UploadManager::StartUploads()
{
    PGP_ZONE_SCOPEDN( "UploadManager::StartUploads" );
    Fence& fence                            = fences[currentBufferIdx];
    std::vector<UploadRequest>& requestList = requests[currentBufferIdx];
    CommandBuffer& cmdBuf                   = cmdBufs[currentBufferIdx];

    if ( requestList.empty() )
        return;

    fence.Reset();
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    size_t stagingOffset  = 0;
    Buffer& stagingBuffer = stagingBuffers[currentBufferIdx];

#if USING( TRANSFER_QUEUE_UPLOADS )
    u32 transferQueueFamilyIndex = rg.device.GetQueue( QueueType::TRANSFER ).familyIndex;
#else  // #if USING( TRANSFER_QUEUE_UPLOADS )
    u32 transferQueueFamilyIndex = rg.device.GetQueue( QueueType::GRAPHICS ).familyIndex;
#endif // #else // #if USING( TRANSFER_QUEUE_UPLOADS )
    u32 mainQueueFamilyIndex = rg.device.GetQueue( QueueType::GRAPHICS ).familyIndex;

    VkBufferMemoryBarrier2 releaseBufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
    releaseBufferBarrier.srcStageMask        = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    releaseBufferBarrier.srcAccessMask       = VK_ACCESS_2_MEMORY_WRITE_BIT;
    releaseBufferBarrier.srcQueueFamilyIndex = transferQueueFamilyIndex;
    releaseBufferBarrier.dstQueueFamilyIndex = mainQueueFamilyIndex;

    VkBufferMemoryBarrier2 acquireBufferBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
    acquireBufferBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT; // VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
    acquireBufferBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
    acquireBufferBarrier.srcQueueFamilyIndex = transferQueueFamilyIndex;
    acquireBufferBarrier.dstQueueFamilyIndex = mainQueueFamilyIndex;

    VkImageMemoryBarrier2 acquireTextureBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    acquireTextureBarrier.dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT; // VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
    acquireTextureBarrier.dstAccessMask       = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    acquireTextureBarrier.srcQueueFamilyIndex = transferQueueFamilyIndex;
    acquireTextureBarrier.dstQueueFamilyIndex = mainQueueFamilyIndex;
    acquireTextureBarrier.oldLayout           = PGToVulkanImageLayout( ImageLayout::TRANSFER_DST );
    acquireTextureBarrier.newLayout           = PGToVulkanImageLayout( ImageLayout::SHADER_READ_ONLY );
    acquireTextureBarrier.subresourceRange    = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );

    VkBufferImageCopy region = {};
    for ( UploadRequest& req : requestList )
    {
        if ( req.type == UploadCommandType::BUFFER_UPLOAD )
        {
            VkBufferCopy copyRegion;
            copyRegion.dstOffset = req.offset;
            copyRegion.srcOffset = stagingOffset;
            copyRegion.size      = req.size;
            vkCmdCopyBuffer( cmdBuf, stagingBuffer, req.buffer, 1, &copyRegion );

#if USING( TRANSFER_QUEUE_UPLOADS )
            releaseBufferBarrier.buffer = req.buffer;
            releaseBufferBarrier.size   = req.size;
            releaseBufferBarrier.offset = req.offset;
            cmdBuf.PipelineBufferBarrier2( releaseBufferBarrier );

            acquireBufferBarrier.buffer = req.buffer;
            acquireBufferBarrier.size   = req.size;
            acquireBufferBarrier.offset = req.offset;
            pendingBufferBarriers.push_back( acquireBufferBarrier );
#endif // #if USING( TRANSFER_QUEUE_UPLOADS )
        }
        else if ( req.type == UploadCommandType::LAYOUT_TRANSITION_PRE )
        {
            cmdBuf.TransitionImageLayout(
                req.image, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST, 0, VK_PIPELINE_STAGE_2_TRANSFER_BIT );
        }
        else if ( req.type == UploadCommandType::LAYOUT_TRANSITION_POST )
        {
            cmdBuf.TransitionImageLayout( req.image, ImageLayout::TRANSFER_DST, ImageLayout::SHADER_READ_ONLY,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, transferQueueFamilyIndex, mainQueueFamilyIndex );
#if USING( TRANSFER_QUEUE_UPLOADS )
            acquireTextureBarrier.image = req.image;
            pendingImageBarriers.push_back( acquireTextureBarrier );
#endif // #if USING( TRANSFER_QUEUE_UPLOADS )
        }
        else if ( req.type == UploadCommandType::TEXTURE_UPLOAD )
        {
            region.bufferOffset     = stagingOffset;
            region.imageSubresource = req.texReq.imageSubresource;
            region.imageExtent      = req.texReq.imageExtent;
            vkCmdCopyBufferToImage( cmdBuf, stagingBuffer, req.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );
        }
        stagingOffset += req.size;
    }
    cmdBuf.EndRecording();
    requestList.clear();
    currentSizes[currentBufferIdx] = 0;

    VkCommandBufferSubmitInfo cmdBufInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    cmdBufInfo.commandBuffer = cmdBuf;

    VkSubmitInfo2 info          = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos    = &cmdBufInfo;

    QueueType queue = USING( TRANSFER_QUEUE_UPLOADS ) ? QueueType::TRANSFER : QueueType::GRAPHICS;
    VK_CHECK( vkQueueSubmit2( rg.device.GetQueue( queue ), 1, &info, fence ) );
}

void UploadManager::SwapBuffers()
{
    StartUploads();
    currentBufferIdx = ( currentBufferIdx + 1 ) % NUM_BUFFERS;
    fences[currentBufferIdx].WaitFor();
}

void UploadManager::FlushAll()
{
    SwapBuffers();
    SwapBuffers();
}

} // namespace PG::Gfx
