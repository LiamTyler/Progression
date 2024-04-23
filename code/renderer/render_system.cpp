#include "render_system.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"

using namespace PG;
using namespace Gfx;

namespace PG::RenderSystem
{

bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless )
{
    rg.sceneWidth  = sceneWidth;
    rg.sceneHeight = sceneHeight;

    if ( !R_Init( headless, sceneWidth, sceneHeight ) )
        return false;

    return true;
}

void Shutdown()
{
    rg.device.WaitForIdle();
    R_Shutdown();
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

    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::UNDEFINED, ImageLayout::GENERAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT );

    // make a clear-color from frame number. This will flash with a 120 frame period.
    VkClearColorValue clearValue;
    float flash = abs( sin( rg.currentFrameIdx / 120.f ) );
    clearValue  = {
        {0.0f, 0.0f, flash, 1.0f}
    };

    VkImageSubresourceRange clearRange = ImageSubresourceRange( VK_IMAGE_ASPECT_COLOR_BIT );

    // clear image
    vkCmdClearColorImage( cmdBuf.GetHandle(), rg.swapchain.GetImage(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange );

    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::GENERAL, ImageLayout::PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_CLEAR_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT );

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
