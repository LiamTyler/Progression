#pragma once

#include "core/input_types.hpp"
#include "shared/math_vec.hpp"

struct lua_State;

namespace PG::Input
{

void Init();
void Shutdown();

// should be called once per frame to update inputs
void PollEvents();

// Get[Key/MouseButton]Down returns true the first frame the Key/MouseButton is pressed
// Get[Key/MouseButton]Up returns true the first frame the Key/MouseButton is released
// Get[Key/MouseButton]Held returns true all frames the Key/MouseButton is pressed, including the first frame
bool GetKeyDown( Key k );
bool AnyKeyDown();
bool GetKeyUp( Key k );
bool AnyKeyUp();
bool GetKeyHeld( Key k );

bool GetMouseButtonDown( MouseButton b );
bool AnyMouseButtonDown();
bool GetMouseButtonUp( MouseButton b );
bool AnyMouseButtonUp();
bool GetMouseButtonHeld( MouseButton b );

vec2 GetMousePosition();
vec2 GetMouseChange();
vec2 GetScrollChange();

void RegisterLuaFunctions( lua_State* L );

} // namespace PG::Input
