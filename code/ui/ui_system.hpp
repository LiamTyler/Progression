#pragma once

#include "ui/ui_element.hpp"
#include "asset/types/ui_layout.hpp"

namespace PG::Gfx
{
    class CommandBuffer;
    class RenderPass;
    class DescriptorSet;
}

namespace PG::UI
{
    bool Init();
    void BootMainMenu();
    void Shutdown();
    void Clear(); // removes all UI elements

    UIElement* CreateElement( UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* CreateChildElement( UIElementHandle parent, UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* GetElement( UIElementHandle handle );
    UIElement* GetChildElement( UIElementHandle parentHandle, uint32_t childIdx );
    void RemoveElement( UIElementHandle handle ); // is recursive
    UIElement* GetLayoutRootElement( const std::string& layoutName );
    bool CreateLayout( const std::string& layoutName );
    void RemoveLayout( const std::string& layoutName );

#if USING( ASSET_LIVE_UPDATE )
    void ReloadLayoutIfInUse( UILayout* oldLayout, UILayout* newLayout );
#endif // #if USING( ASSET_LIVE_UPDATE )

    uint16_t AddScript( const std::string& scriptName );

    void Update();
    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet );

} // namespace PG::UI

