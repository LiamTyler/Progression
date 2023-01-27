#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG::Gfx
{
    class CommandBuffer;
    class RenderPass;
    class DescriptorSet;
}

namespace PG::UI
{

    enum class UIElementFlags : uint32_t
    {
        ACTIVE  = (1u << 0),
        VISIBLE = (1u << 1),
    };
    PG_DEFINE_ENUM_OPS( UIElementFlags );

    enum class ElementBlendMode : uint8_t
    {
        OPAQUE = 0,
        BLEND = 1,
        ADDITIVE = 2,

        COUNT = 3
    };

    using UIElementHandle = uint16_t;
    static constexpr UIElementHandle UI_NULL_HANDLE = UINT16_MAX;

    struct UIElement
    {
        UIElementHandle parent;
        UIElementHandle prevSibling;
        UIElementHandle nextSibling;
        UIElementHandle firstChild;
        UIElementHandle lastChild;

        UIElementFlags flags = UIElementFlags::VISIBLE;
        ElementBlendMode blendMode = ElementBlendMode::OPAQUE;
        glm::vec2 pos; // normalized 0 - 1
        glm::vec2 dimensions; // normalized 0 - 1
        glm::vec4 tint;
        GfxImage *image = nullptr;

        UIElementHandle Handle() const;
    };

    bool Init( Gfx::RenderPass* renderPass );
    void Shutdown();
    void Clear(); // removes all UI elements

    UIElement* CreateElement( UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* CreateChildElement( UIElementHandle parent, UIElementHandle templateElement = UI_NULL_HANDLE );
    UIElement* GetElement( UIElementHandle handle );
    void RemoveElement( UIElementHandle handle ); // is recursive

    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet );

} // namespace PG::UI

