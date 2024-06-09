#pragma once

#include "shared/platform_defines.hpp"

namespace PG::Input
{

enum class Action : u16
{
#if !USING( SHIP_BUILD )
    TOGGLE_DEV_CONSOLE,
    TOGGLE_DEBUG_UI,
#endif // #if !USING( SHIP_BUILD )
    QUIT_GAME,
    TOGGLE_CAMERA_CONTROLS,
    CAMERA_SPRINT,

    COUNT
};

const char* ActionToString( Action action );

enum class ActionState : u8
{
    PRESSED,  // only true the first frame pressed
    HELD,     // true for the 2nd frame and onwards that an action is pressed
    RELEASED, // only true the first frame released
};

struct ActionStatePair
{
    Action action;
    ActionState state;
};

enum class Axis : u8
{
    CAMERA_VEL_X,
    CAMERA_VEL_Y,
    CAMERA_VEL_Z,
    CAMERA_YAW,
    CAMERA_PITCH,

    COUNT
};

const char* AxisToString( Axis axis );

using AxisValue = f32;

struct AxisValuePair
{
    Axis axis;
    AxisValue value;
};

} // namespace PG::Input
