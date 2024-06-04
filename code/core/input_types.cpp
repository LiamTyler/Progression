#include "input_types.hpp"

namespace PG::Input
{

const char* ActionToString( Action action )
{
    static const char* names[] = {
#if !USING( SHIP_BUILD )
        "TOGGLE_DEV_CONSOLE",
        "TOGGLE_DEBUG_UI",
#endif // #if !USING( SHIP_BUILD )
        "QUIT_GAME",
        "TOGGLE_CAMERA_CONTROLS",
        "CAMERA_SPRINT",
    };
    static_assert( Underlying( Action::COUNT ) == ARRAY_COUNT( names ), "don't forget to update" );

    return names[Underlying( action )];
}

const char* AxisToString( Axis axis )
{
    static const char* names[] = {
        "CAMERA_VEL_X",
        "CAMERA_VEL_Y",
        "CAMERA_VEL_Z",
        "CAMERA_YAW",
        "CAMERA_PITCH",
    };
    static_assert( Underlying( Axis::COUNT ) == ARRAY_COUNT( names ), "don't forget to update" );

    return names[Underlying( axis )];
}

} // namespace PG::Input
