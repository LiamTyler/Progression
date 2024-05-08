#include "r_tg_tasks.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/taskgraph/r_taskGraph.hpp"

using std::vector;

namespace PG::Gfx
{

static void FillBufferBarriers( TaskGraph* taskGraph, vector<VkBufferMemoryBarrier2>& scratch, vector<VkBufferMemoryBarrier2>& source )
{
    scratch = source;
    for ( VkBufferMemoryBarrier2& barrier : scratch )
    {
        TGResourceHandle bufHandle;
        memcpy( &bufHandle, &barrier.buffer, sizeof( TGResourceHandle ) );
        barrier.buffer = taskGraph->GetBuffer( bufHandle )->GetHandle();
    }
}

static void FillImageBarriers( TaskGraph* taskGraph, vector<VkImageMemoryBarrier2>& scratch, vector<VkImageMemoryBarrier2>& source )
{
    scratch = source;
    for ( VkImageMemoryBarrier2& barrier : scratch )
    {
        TGResourceHandle imgHandle;
        memcpy( &imgHandle, &barrier.image, sizeof( TGResourceHandle ) );
        barrier.image = taskGraph->GetTexture( imgHandle )->GetImage();
    }
}

static void FillAttachmentInfos( TaskGraph* taskGraph, vector<VkRenderingAttachmentInfo>& scratch, vector<VkRenderingAttachmentInfo>& source )
{
    scratch = source;
    for ( VkRenderingAttachmentInfo& attach : scratch )
    {
        TGResourceHandle imgHandle;
        memcpy( &imgHandle, &attach.imageView, sizeof( TGResourceHandle ) );
        attach.imageView = taskGraph->GetTexture( imgHandle )->GetView();
    }
}

void Task::SubmitBarriers( TGExecuteData* data )
{
    if ( imageBarriers.empty() && bufferBarriers.empty() )
        return;

    FillImageBarriers( data->taskGraph, data->scratchImageBarriers, imageBarriers );
    FillBufferBarriers( data->taskGraph, data->scratchBufferBarriers, bufferBarriers );

    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount  = (uint32_t)data->scratchImageBarriers.size();
    depInfo.pImageMemoryBarriers     = data->scratchImageBarriers.data();
    depInfo.bufferMemoryBarrierCount = (uint32_t)data->scratchBufferBarriers.size();
    depInfo.pBufferMemoryBarriers    = data->scratchBufferBarriers.data();

    vkCmdPipelineBarrier2( *data->cmdBuf, &depInfo );
}

void ComputeTask::Execute( TGExecuteData* data )
{
    if ( !imageBarriersPreClears.empty() )
    {
        FillImageBarriers( data->taskGraph, data->scratchImageBarriers, imageBarriersPreClears );
        VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depInfo.imageMemoryBarrierCount = (uint32_t)data->scratchImageBarriers.size();
        depInfo.pImageMemoryBarriers    = data->scratchImageBarriers.data();

        vkCmdPipelineBarrier2( *data->cmdBuf, &depInfo );
    }

    for ( const BufferClearSubTask& clear : bufferClears )
    {
        Buffer* buf = data->taskGraph->GetBuffer( clear.bufferHandle );
        vkCmdFillBuffer( *data->cmdBuf, buf->GetHandle(), 0, VK_WHOLE_SIZE, clear.clearVal );
    }
    for ( const TextureClearSubTask& clear : textureClears )
    {
        Texture* tex = data->taskGraph->GetTexture( clear.textureHandle );
        VkClearColorValue clearColor;
        memcpy( &clearColor, &clear.clearVal.x, sizeof( vec4 ) );
        VkImageSubresourceRange range = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );
        vkCmdClearColorImage( *data->cmdBuf, tex->GetImage(), PGToVulkanImageLayout( ImageLayout::TRANSFER_DST ), &clearColor, 1, &range );
    }

    SubmitBarriers( data );

    function( this, data );
}

void GraphicsTask::Execute( TGExecuteData* data )
{
    SubmitBarriers( data );
    
    FillAttachmentInfos( data->taskGraph, data->scratchAttachmentInfos, attachments );
    renderingInfo.pColorAttachments = data->scratchAttachmentInfos.data();

    vkCmdBeginRendering( *data->cmdBuf, &renderingInfo );
    function( this, data );
    vkCmdEndRendering( *data->cmdBuf );
}

void TransferTask::Execute( TGExecuteData* data )
{
    SubmitBarriers( data );

    for ( const TextureTransfer& texBlit : textureBlits )
    {
        Texture* srcTex = data->taskGraph->GetTexture( texBlit.src );
        Texture* dstTex = data->taskGraph->GetTexture( texBlit.dst );

        VkImageBlit2 blitRegion{ VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };
        blitRegion.srcOffsets[1].x = srcTex->GetWidth();
        blitRegion.srcOffsets[1].y = srcTex->GetHeight();
        blitRegion.srcOffsets[1].z = 1;

        blitRegion.dstOffsets[1].x = dstTex->GetWidth();
        blitRegion.dstOffsets[1].y = dstTex->GetHeight();
        blitRegion.dstOffsets[1].z = 1;

        blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.srcSubresource.baseArrayLayer = 0;
        blitRegion.srcSubresource.layerCount     = 1;
        blitRegion.srcSubresource.mipLevel       = 0;

        blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blitRegion.dstSubresource.baseArrayLayer = 0;
        blitRegion.dstSubresource.layerCount     = 1;
        blitRegion.dstSubresource.mipLevel       = 0;

        VkBlitImageInfo2 blitInfo{ VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2 };
        blitInfo.dstImage       = dstTex->GetImage();
        blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitInfo.srcImage       = srcTex->GetImage();
        blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitInfo.filter         = VK_FILTER_LINEAR;
        blitInfo.regionCount    = 1;
        blitInfo.pRegions       = &blitRegion;

        vkCmdBlitImage2( *data->cmdBuf, &blitInfo );
    }
}

void PresentTask::Execute( TGExecuteData* data )
{
    SubmitBarriers( data );
}

} // namespace PG::Gfx
