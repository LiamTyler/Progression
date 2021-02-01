#pragma once

#include "core/input_types.hpp"
#include "glm/vec2.hpp"

struct lua_State;

namespace PG
{
namespace Input
{

    void Init();
    void Shutdown();

    // should be called once per frame to update inputs
    void PollEvents();

    // Get[Key/MouseButton]Down returns true the first frame the Key/MouseButton is pressed
    // Get[Key/MouseButton]Up returns true the first frame the Key/MouseButton is released
    // Get[Key/MouseButton]Held returns true all frames the Key/MouseButton is pressed, including the first frame

    bool GetKeyDown( Key k );
    bool GetKeyUp( Key k );
    bool GetKeyHeld( Key k );
    bool GetMouseButtonDown( MouseButton b );
    bool GetMouseButtonUp( MouseButton b );
    bool GetMouseButtonHeld( MouseButton b ); 
    glm::vec2 GetMousePosition();
    glm::vec2 GetMouseChange();
    glm::vec2 GetScrollChange();

    void RegisterLuaFunctions( lua_State* L );

} // namespace Input
} // namespace PG
