#pragma once

#include "asset/types/ui_layout.hpp"
#include "ui/ui_element.hpp"

namespace PG::Gfx
{
class CommandBuffer;
} // namespace PG::Gfx

namespace PG::UI
{

bool Init();
void BootMainMenu();
void Shutdown();
void Clear(); // removes all UI elements

UIElement* CreateElement( UIElementHandle templateElement = UI_NULL_HANDLE );
UIElement* CreateChildElement( UIElementHandle parent, UIElementHandle templateElement = UI_NULL_HANDLE );
UIElement* GetElement( UIElementHandle handle );
UIElement* GetChildElement( UIElementHandle parentHandle, u32 childIdx );
void RemoveElement( UIElementHandle handle ); // is recursive
UIElement* GetLayoutRootElement( const std::string& layoutName );
bool CreateLayout( const std::string& layoutName );
void RemoveLayout( const std::string& layoutName );

#if USING( ASSET_LIVE_UPDATE )
void ReloadScriptIfInUse( Script* oldScript, Script* newScript );
void ReloadLayoutIfInUse( UILayout* oldLayout, UILayout* newLayout );
#endif // #if USING( ASSET_LIVE_UPDATE )

u16 AddScript( const std::string& scriptName );

void Update();
void Render( Gfx::CommandBuffer* cmdBuf );

} // namespace PG::UI
