#pragma once

#include "core/feature_defines.hpp"

#include "imgui/imgui.h"
#include "renderer/graphics_api/command_buffer.hpp"
#include <functional>

namespace PG::Gfx::UIOverlay
{

bool Init( PixelFormat colorAttachmentFormat );
void Shutdown();

void BeginFrame();
void Render( CommandBuffer& cmdBuf );
void EndFrame();

void AddDrawFunction( const std::function<void()>& func );
bool CapturingMouse(); // true if cursor is ontop of UI element
void ToggleConsoleVisibility();

bool Header( const char* caption );
bool CheckBox( const char* caption, bool* value );
bool CheckBox( const char* caption, i32* value );
bool InputFloat( const char* caption, f32* value, f32 step );
bool SliderFloat( const char* caption, f32* value, f32 min, f32 max );
bool SliderInt( const char* caption, i32* value, i32 min, i32 max );
bool ComboBox( const char* caption, i32* itemindex, const std::vector<std::string>& items );
bool Button( const char* caption );
void Text( const char* formatstr, ... );

bool Updated();

} // namespace PG::Gfx::UIOverlay
