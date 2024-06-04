#pragma once

#include "shared/platform_defines.hpp"

namespace PG::Input
{

enum class Action : uint16_t
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

enum class ActionState : uint8_t
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

enum class Axis : uint8_t
{
    CAMERA_VEL_X,
    CAMERA_VEL_Y,
    CAMERA_VEL_Z,
    CAMERA_YAW,
    CAMERA_PITCH,

    COUNT
};

const char* AxisToString( Axis axis );

using AxisValue = float;

struct AxisValuePair
{
    Axis axis;
    AxisValue value;
};

} // namespace PG::Input
