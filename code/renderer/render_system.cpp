#include "render_system.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

namespace PG::RenderSystem
{

Texture s_drawImg;

bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless )
{
    rg.sceneWidth  = sceneWidth;
    rg.sceneHeight = sceneHeight;

    if ( !R_Init( headless, sceneWidth, sceneHeight ) )
        return false;

    TextureCreateInfo texInfo( PixelFormat::R16_G16_B16_A16_FLOAT, sceneWidth, sceneHeight );
    texInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    s_drawImg = rg.device.NewTexture( texInfo, "sceneTex" );

    return true;
}

void Shutdown()
{
    rg.device.WaitForIdle();
    s_drawImg.Free();
    R_Shutdown();
}

void copy_image_to_image( VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize )
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.dstSubresource.mipLevel       = 0;

    VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blitInfo.dstImage       = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage       = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter         = VK_FILTER_LINEAR;
    blitInfo.regionCount    = 1;
    blitInfo.pRegions       = &blitRegion;

    vkCmdBlitImage2( cmd, &blitInfo );
}

void Render()
{
    FrameData& frameData = rg.GetFrameData();
    frameData.renderingCompleteFence.WaitFor();
    frameData.renderingCompleteFence.Reset();
    if ( !rg.headless )
        rg.swapchain.AcquireNextImage( frameData.swapchainSemaphore );

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    cmdBuf.TransitionImageLayout( s_drawImg.GetImage(), ImageLayout::UNDEFINED, ImageLayout::GENERAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT );

    // make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = abs( sin( rg.currentFrameIdx / 120.f ) );
    clearValue  = {
        {0.0f, 0.0f, flash, 1.0f}
    };

    VkImageSubresourceRange clearRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );

    // clear image
    vkCmdClearColorImage( cmdBuf.GetHandle(), s_drawImg.GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange );

    cmdBuf.TransitionImageLayout( s_drawImg.GetImage(), ImageLayout::GENERAL, ImageLayout::TRANSFER_SRC,
        VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT );
    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT );

    VkExtent2D swpExt = { rg.swapchain.GetWidth(), rg.swapchain.GetHeight() };
    copy_image_to_image( cmdBuf.GetHandle(), s_drawImg.GetImage(), rg.swapchain.GetImage(), s_drawImg.GetExtent2D(), swpExt );

    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::TRANSFER_DST, ImageLayout::PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT );



    cmdBuf.EndRecording();

    VkSemaphoreSubmitInfo waitInfo =
        SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, frameData.swapchainSemaphore.GetHandle() );
    VkSemaphoreSubmitInfo signalInfo =
        SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameData.renderingCompleteSemaphore.GetHandle() );
    rg.device.Submit( cmdBuf, &waitInfo, &signalInfo, &frameData.renderingCompleteFence );

    if ( !rg.headless )
        rg.device.Present( rg.swapchain, frameData.renderingCompleteSemaphore );

    ++rg.currentFrameIdx;
}

void CreateTLAS( Scene* scene ) {}
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name ) { return {}; }

} // namespace PG::RenderSystem
