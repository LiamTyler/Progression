#include "r_globals.hpp"

namespace PG::Gfx
{

R_Globals rg;

void R_Globals::SubmitImmediateCommandBuffer()
{
    rg.device.Submit( QueueType::GRAPHICS, immediateCmdBuffer, nullptr, nullptr, &immediateFence );
    rg.immediateFence.WaitFor();
}

Viewport DisplaySizedViewport( bool vulkanFlipViewport )
{
    Viewport v( static_cast<f32>( rg.swapchain.GetWidth() ), static_cast<f32>( rg.swapchain.GetHeight() ) );
    if ( vulkanFlipViewport )
    {
        v.y = v.height;
        v.height *= -1;
    }
    return v;
}

Viewport SceneSizedViewport( bool vulkanFlipViewport )
{
    Viewport v( static_cast<f32>( rg.sceneWidth ), static_cast<f32>( rg.sceneHeight ) );
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
