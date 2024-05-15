#include "r_globals.hpp"

namespace PG::Gfx
{

R_Globals rg;

void R_Globals::SubmitImmediateCommandBuffer()
{
    VkCommandBufferSubmitInfo cmdBufInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
    cmdBufInfo.commandBuffer = rg.immediateCmdBuffer;

    VkSubmitInfo2 info          = { VK_STRUCTURE_TYPE_SUBMIT_INFO_2 };
    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos    = &cmdBufInfo;

    VK_CHECK( vkQueueSubmit2( rg.device.GetQueue(), 1, &info, immediateFence ) );
    rg.immediateFence.WaitFor();
}

Viewport DisplaySizedViewport( bool vulkanFlipViewport )
{
    Viewport v( static_cast<float>( rg.swapchain.GetWidth() ), static_cast<float>( rg.swapchain.GetHeight() ) );
    if ( vulkanFlipViewport )
    {
        v.y = v.height;
        v.height *= -1;
    }
    return v;
}

Viewport SceneSizedViewport( bool vulkanFlipViewport )
{
    Viewport v( static_cast<float>( rg.sceneWidth ), static_cast<float>( rg.sceneHeight ) );
    if ( vulkanFlipViewport )
    {
        v.y = v.height;
        v.height *= -1;
    }
    return v;
}

Scissor DisplaySizedScissor() { return Scissor( rg.swapchain.GetWidth(), rg.swapchain.GetHeight() ); }

Scissor SceneSizedScissor() { return Scissor( rg.sceneWidth, rg.sceneHeight ); }

} // namespace PG::Gfx
