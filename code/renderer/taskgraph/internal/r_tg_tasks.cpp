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
        TGResourceHandle bufHandle = GetEmbeddedResHandle( barrier.buffer );
        barrier.buffer             = *taskGraph->GetBuffer( bufHandle );
    }
}

static void FillImageBarriers( TaskGraph* taskGraph, vector<VkImageMemoryBarrier2>& scratch, vector<VkImageMemoryBarrier2>& source )
{
    scratch = source;
    for ( VkImageMemoryBarrier2& barrier : scratch )
    {
        TGResourceHandle imgHandle = GetEmbeddedResHandle( barrier.image );
        barrier.image              = taskGraph->GetTexture( imgHandle )->GetImage();
    }
}

static void FillAttachmentInfos(
    TaskGraph* taskGraph, vector<VkRenderingAttachmentInfo>& scratch, vector<VkRenderingAttachmentInfo>& source )
{
    scratch = source;
    for ( VkRenderingAttachmentInfo& attach : scratch )
    {
        TGResourceHandle imgHandle = GetEmbeddedResHandle( attach.imageView );
        attach.imageView           = taskGraph->GetTexture( imgHandle )->GetView();
    }
}

void Task::SubmitBarriers( TGExecuteData* data )
{
    if ( imageBarriers.empty() && bufferBarriers.empty() )
        return;

    FillImageBarriers( data->taskGraph, data->scratchImageBarriers, imageBarriers );
    FillBufferBarriers( data->taskGraph, data->scratchBufferBarriers, bufferBarriers );

    VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    depInfo.imageMemoryBarrierCount  = (u32)data->scratchImageBarriers.size();
    depInfo.pImageMemoryBarriers     = data->scratchImageBarriers.data();
    depInfo.bufferMemoryBarrierCount = (u32)data->scratchBufferBarriers.size();
    depInfo.pBufferMemoryBarriers    = data->scratchBufferBarriers.data();

    vkCmdPipelineBarrier2( *data->cmdBuf, &depInfo );
}

void ComputeTask::Execute( TGExecuteData* data )
{
    if ( !imageBarriersPreClears.empty() )
    {
        FillImageBarriers( data->taskGraph, data->scratchImageBarriers, imageBarriersPreClears );
        VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        depInfo.imageMemoryBarrierCount = (u32)data->scratchImageBarriers.size();
        depInfo.pImageMemoryBarriers    = data->scratchImageBarriers.data();

        vkCmdPipelineBarrier2( *data->cmdBuf, &depInfo );
    }

    for ( const BufferClearSubTask& clear : bufferClears )
    {
        Buffer* buf = data->taskGraph->GetBuffer( clear.bufferHandle );
        vkCmdFillBuffer( *data->cmdBuf, *buf, 0, VK_WHOLE_SIZE, clear.clearVal );
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

GraphicsTask::~GraphicsTask()
{
    if ( depthAttach )
        delete depthAttach;
}

void GraphicsTask::Execute( TGExecuteData* data )
{
    SubmitBarriers( data );

    FillAttachmentInfos( data->taskGraph, data->scratchColorAttachmentInfos, colorAttachments );
    renderingInfo.pColorAttachments = data->scratchColorAttachmentInfos.data();
    if ( depthAttach )
    {
        data->scratchDepthAttachmentInfo           = *depthAttach;
        TGResourceHandle imgHandle                 = GetEmbeddedResHandle( depthAttach->imageView );
        data->scratchDepthAttachmentInfo.imageView = data->taskGraph->GetTexture( imgHandle )->GetView();
        renderingInfo.pDepthAttachment             = &data->scratchDepthAttachmentInfo;
    }

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

void PresentTask::Execute( TGExecuteData* data ) { SubmitBarriers( data ); }

#if USING( TG_DEBUG )
template <u32 N>
constexpr u32 StrLen( char const ( & )[N] )
{
    return N - 1;
}

static const char* ImgLayoutToString( VkImageLayout layout )
{
    // ex: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
    constexpr u32 L = StrLen( "VK_IMAGE_LAYOUT_" );
    return string_VkImageLayout( layout ) + L;
}

static const char* PipelineStageFlagsToString( VkPipelineStageFlags2 stage )
{
    if ( stage == VK_PIPELINE_STAGE_2_NONE )
        return "NONE";

    // ex: VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT -> ALL_TRANSFER_BIT
    constexpr u32 L = StrLen( "VK_PIPELINE_STAGE_2_" );
    return string_VkPipelineStageFlags2( stage ).c_str() + L;
}

// todo: work with or-ed flags |
static const char* AccessFlagsToString( VkAccessFlags2 flags )
{
    if ( flags == VK_ACCESS_2_NONE )
        return "NONE";

    // ex: VK_ACCESS_2_SHADER_WRITE_BIT -> SHADER_WRITE_BIT
    constexpr u32 L = StrLen( "VK_ACCESS_2_" );
    return string_VkAccessFlags2( flags ).c_str() + L;
}

static const char* LoadOpToString( VkAttachmentLoadOp op )
{
    // ex: VK_ATTACHMENT_LOAD_OP_LOAD -> LOAD
    constexpr u32 L = StrLen( "VK_ATTACHMENT_LOAD_OP_" );
    return string_VkAttachmentLoadOp( op ) + L;
}

static const char* StoreOpToString( VkAttachmentStoreOp op )
{
    // ex: VK_ATTACHMENT_STORE_OP_STORE -> STORE
    constexpr u32 L = StrLen( "VK_ATTACHMENT_STORE_OP_" );
    return string_VkAttachmentStoreOp( op ) + L;
}

static void PrintImageBarrier( std::string_view indent, const VkImageMemoryBarrier2& barrier )
{
    LOG( "%sSRCStage:  %s", indent.data(), PipelineStageFlagsToString( barrier.srcStageMask ) );
    LOG( "%sSRCAccess: %s", indent.data(), AccessFlagsToString( barrier.srcAccessMask ) );
    LOG( "%sDSTStage:  %s", indent.data(), PipelineStageFlagsToString( barrier.dstStageMask ) );
    LOG( "%sDSTAccess: %s", indent.data(), AccessFlagsToString( barrier.dstAccessMask ) );
    LOG( "%sOldLayout: %s", indent.data(), ImgLayoutToString( barrier.oldLayout ) );
    LOG( "%sNewLayout: %s", indent.data(), ImgLayoutToString( barrier.newLayout ) );
}

void Task::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Buffer Barriers: %zu", bufferBarriers.size() );
    for ( size_t i = 0; i < bufferBarriers.size(); ++i )
    {
        const VkBufferMemoryBarrier2& barrier = bufferBarriers[i];
        TGResourceHandle bufHandle            = GetEmbeddedResHandle( barrier.buffer );
        LOG( "      [%zu]: Buffer: %u '%s'", i, bufHandle, taskGraph->GetBuffer( bufHandle )->GetDebugName() );
        LOG( "        SRCStage:  %s", PipelineStageFlagsToString( barrier.srcStageMask ) );
        LOG( "        SRCAccess: %s", AccessFlagsToString( barrier.srcAccessMask ) );
        LOG( "        DSTStage:  %s", PipelineStageFlagsToString( barrier.dstStageMask ) );
        LOG( "        DSTAccess: %s", AccessFlagsToString( barrier.dstAccessMask ) );
    }

    LOG( "    Image Barriers: %zu", imageBarriers.size() );
    for ( size_t i = 0; i < imageBarriers.size(); ++i )
    {
        const VkImageMemoryBarrier2& barrier = imageBarriers[i];
        TGResourceHandle imgHandle           = GetEmbeddedResHandle( barrier.image );
        LOG( "      [%zu]: Image: %u '%s'", i, imgHandle, taskGraph->GetTexture( imgHandle )->GetDebugName() );
        PrintImageBarrier( "        ", barrier );
    }
}

void PipelineTask::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Pre-Clear Image Barriers: %zu", imageBarriersPreClears.size() );
    for ( size_t i = 0; i < imageBarriersPreClears.size(); ++i )
    {
        const VkImageMemoryBarrier2& barrier = imageBarriersPreClears[i];
        TGResourceHandle imgHandle           = GetEmbeddedResHandle( barrier.image );
        LOG( "      [%zu]: Image: %u '%s'", i, imgHandle, taskGraph->GetTexture( imgHandle )->GetDebugName() );
        PrintImageBarrier( "        ", barrier );
    }
    Task::Print( taskGraph );

    LOG( "    Buffer Clears: %zu", bufferClears.size() );
    for ( size_t i = 0; i < bufferClears.size(); ++i )
    {
        const BufferClearSubTask& c = bufferClears[i];
        const Buffer* buf           = taskGraph->GetBuffer( c.bufferHandle );
        LOG( "      [%zu]: Buffer %u ('%s'), Clear Val: %u", i, c.bufferHandle, buf->GetDebugName(), c.clearVal );
    }

    LOG( "    Texture Clears: %zu", textureClears.size() );
    for ( size_t i = 0; i < textureClears.size(); ++i )
    {
        const TextureClearSubTask& c = textureClears[i];
        const Texture* tex           = taskGraph->GetTexture( c.textureHandle );
        LOG( "      [%zu]: Texture %u ('%s'), Clear Color: %g %g %g %g", i, c.textureHandle, tex->GetDebugName(), c.clearVal.r,
            c.clearVal.g, c.clearVal.b, c.clearVal.a );
    }

    LOG( "    Input Buffers: %zu", inputBuffers.size() );
    for ( size_t i = 0; i < inputBuffers.size(); ++i )
    {
        const Buffer* buf = taskGraph->GetBuffer( inputBuffers[i] );
        LOG( "      [%zu]: Buffer %u ('%s')", i, inputBuffers[i], buf->GetDebugName() );
    }

    LOG( "    Output Buffers: %zu", outputBuffers.size() );
    for ( size_t i = 0; i < outputBuffers.size(); ++i )
    {
        const Buffer* buf = taskGraph->GetBuffer( outputBuffers[i] );
        LOG( "      [%zu]: Buffer %u ('%s')", i, outputBuffers[i], buf->GetDebugName() );
    }

    LOG( "    Input Textures: %zu", inputTextures.size() );
    for ( size_t i = 0; i < inputTextures.size(); ++i )
    {
        const Texture* tex = taskGraph->GetTexture( inputTextures[i] );
        LOG( "      [%zu]: Texture %u ('%s')", i, inputTextures[i], tex->GetDebugName() );
    }

    LOG( "    Output Textures: %zu", outputTextures.size() );
    for ( size_t i = 0; i < outputTextures.size(); ++i )
    {
        const Texture* tex = taskGraph->GetTexture( outputTextures[i] );
        LOG( "      [%zu]: Texture %u ('%s')", i, outputTextures[i], tex->GetDebugName() );
    }
}

void ComputeTask::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Task Type: Compute" );
    PipelineTask::Print( taskGraph );
}

void GraphicsTask::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Task Type: Graphics" );
    PipelineTask::Print( taskGraph );
    LOG( "    Num Color Attachments: %zu", colorAttachments.size() );
    for ( size_t i = 0; i < colorAttachments.size(); ++i )
    {
        const VkRenderingAttachmentInfo& attach = colorAttachments[i];
        TGResourceHandle imgHandle              = GetEmbeddedResHandle( attach.imageView );
        const Texture* tex                      = taskGraph->GetTexture( imgHandle );
        LOG( "      [%zu]: Image: %u '%s'", i, imgHandle, tex->GetDebugName() );
        LOG( "      Layout: %s", ImgLayoutToString( attach.imageLayout ) );
        LOG( "      LoadOp:  %s", LoadOpToString( attach.loadOp ) );
        if ( attach.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR )
        {
            VkClearColorValue c = attach.clearValue.color;
            LOG( "      Clear Color: %g %g %g %g", c.float32[0], c.float32[1], c.float32[2], c.float32[3] );
        }
        LOG( "      StoreOp: %s", StoreOpToString( attach.storeOp ) );
    }

    if ( depthAttach )
    {
        LOG( "    Depth Attachment: Yes" );
        const VkRenderingAttachmentInfo& attach = *depthAttach;
        TGResourceHandle imgHandle              = GetEmbeddedResHandle( attach.imageView );
        const Texture* tex                      = taskGraph->GetTexture( imgHandle );
        LOG( "      Image: %u '%s'", imgHandle, tex->GetDebugName() );
        LOG( "      Layout: %s", ImgLayoutToString( attach.imageLayout ) );
        LOG( "      LoadOp:  %s", LoadOpToString( attach.loadOp ) );
        if ( attach.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR )
        {
            LOG( "      Depth Clear Val: %g", attach.clearValue.depthStencil.depth );
        }
        LOG( "      StoreOp: %s", StoreOpToString( attach.storeOp ) );
    }
    else
    {
        LOG( "    Depth Attachment: No" );
    }
}

void TransferTask::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Task Type: Transfer" );
    Task::Print( taskGraph );
    LOG( "    Texture Blits: %zu", textureBlits.size() );
    for ( size_t i = 0; i < textureBlits.size(); ++i )
    {
        const TextureTransfer& t = textureBlits[i];
        const Texture* src       = taskGraph->GetTexture( t.src );
        const Texture* dst       = taskGraph->GetTexture( t.dst );
        LOG( "      [%zu]: %u -> %u ('%s' -> '%s')", i, t.src, t.dst, src->GetDebugName(), dst->GetDebugName() );
    }
}

void PresentTask::Print( TaskGraph* taskGraph ) const
{
    LOG( "    Task Type: Present" );
    Task::Print( taskGraph );
}
#endif // #if USING( TG_DEBUG )

} // namespace PG::Gfx
