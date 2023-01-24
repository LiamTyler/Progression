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

    struct UIElemenet
    {
        UIElementFlags flags = UIElementFlags::VISIBLE;
        glm::vec2 pos; // normalized 0 - 1
        glm::vec2 dimensions; // normalized 0 - 1
        glm::vec4 tint;
        GfxImage *image = nullptr;
    };

    using UIElementHandle = uint16_t;
    static constexpr UIElementHandle UI_NULL_HANDLE = UINT16_MAX;

    bool Init( Gfx::RenderPass* renderPass );
    void Shutdown();
    void Clear(); // removes all UI elements

    UIElementHandle AddElement( const UIElemenet &element );
    UIElemenet* GetElement( UIElementHandle handle );
    void RemoveElement( UIElementHandle handle );

    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet );

} // namespace PG::UI

