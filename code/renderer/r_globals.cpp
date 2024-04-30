#include "r_globals.hpp"

namespace PG::Gfx
{

R_Globals rg;

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
