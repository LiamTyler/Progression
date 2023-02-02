#pragma once

#include "ui/ui_element.hpp"

namespace PG::Gfx
{
    class CommandBuffer;
    class RenderPass;
    class DescriptorSet;
}

namespace PG::UI
{

    bool Init( Gfx::RenderPass* renderPass );
    void Shutdown();
    void Clear(); // removes all UI elements

    UIElement* CreateElement( UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* CreateChildElement( UIElementHandle parent, UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* GetElement( UIElementHandle handle );
    void RemoveElement( UIElementHandle handle ); // is recursive

    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet );

} // namespace PG::UI

