#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG::UI
{

    // read + write flags (settings) that users can adjust about the UIElement at runtime
    enum class UIElementUserFlags : uint8_t
    {
        NONE              = 0,
        VISIBLE           = (1u << 0),
        APPLY_TONEMAPPING = (1u << 1),
    };
    PG_DEFINE_ENUM_OPS( UIElementUserFlags );

    enum class UIElementReadOnlyFlags : uint8_t
    {
        NONE              = 0,
        HAS_UPDATE_FUNC   = (1u << 0),
    };
    PG_DEFINE_ENUM_OPS( UIElementReadOnlyFlags );

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

        uint16_t scriptFunctionIdx;

        UIElementUserFlags userFlags = UIElementUserFlags::VISIBLE;
        UIElementReadOnlyFlags readOnlyFlags = UIElementReadOnlyFlags::NONE;
        ElementBlendMode blendMode = ElementBlendMode::OPAQUE;
        glm::vec2 pos; // normalized 0 - 1
        glm::vec2 dimensions; // normalized 0 - 1
        glm::vec4 tint;
        GfxImage *image = nullptr;

        UIElementHandle Handle() const;
    };

    constexpr int AAAAAA = sizeof( UIElement );

} // namespace PG::UI

