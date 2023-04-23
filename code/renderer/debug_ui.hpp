#pragma once

#include "core/feature_defines.hpp"
#if USING( PG_DEBUG_UI )

#include "imgui/imgui.h"
#include "renderer/graphics_api/command_buffer.hpp"
#include <functional>

namespace PG::Gfx::UIOverlay
{

    bool Init( const RenderPass& renderPass );
    void Shutdown();

    void BeginFrame();
    void Render( CommandBuffer& cmdBuf );
    void EndFrame();

    void AddDrawFunction( const std::function<void()>& func );
    bool CapturingMouse(); // true if cursor is ontop of UI element

    bool Header( const char* caption );
	bool CheckBox( const char* caption, bool* value );
	bool CheckBox( const char* caption, int* value );
	bool InputFloat( const char* caption, float* value, float step );
	bool SliderFloat( const char* caption, float* value, float min, float max );
	bool SliderInt( const char* caption, int* value, int min, int max );
	bool ComboBox( const char* caption, int* itemindex, const std::vector<std::string>& items );
	bool Button( const char* caption );
	void Text( const char* formatstr, ... );

    bool Updated();

} // namespace PG::Gfx::UIOverlay
#endif // #if USING( PG_DEBUG_UI )