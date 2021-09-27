#include "r_globals.hpp"

namespace PG
{
namespace Gfx
{

    R_Globals r_globals;

    Viewport DisplaySizedViewport( bool vulkanFlipViewport )
    {
        Viewport v( static_cast< float >( r_globals.swapchain.GetWidth() ), static_cast< float >( r_globals.swapchain.GetHeight() ) );
        if ( vulkanFlipViewport )
        {
	        v.y = v.height;
            v.height *= -1;
        }
        return v;
    }


    Viewport SceneSizedViewport( bool vulkanFlipViewport )
    {
        Viewport v( static_cast< float >( r_globals.sceneWidth ), static_cast< float >( r_globals.sceneHeight ) );
        if ( vulkanFlipViewport )
        {
	        v.y = v.height;
            v.height *= -1;
        }
        return v;
    }


    Scissor DisplaySizedScissor()
    {
        return Scissor( r_globals.swapchain.GetWidth(), r_globals.swapchain.GetHeight() );
    }


    Scissor SceneSizedScissor()
    {
        return Scissor( r_globals.sceneWidth, r_globals.sceneHeight );
    }

} // namespace Gfx
} // namespace PG