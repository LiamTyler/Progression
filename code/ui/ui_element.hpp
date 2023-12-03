#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG::UI
{

// read + write flags (settings) that users can adjust about the UIElement at runtime
enum class UIElementUserFlags : uint8_t
{
    NONE              = 0,
    VISIBLE           = ( 1u << 0 ),
    APPLY_TONEMAPPING = ( 1u << 1 ),
};
PG_DEFINE_ENUM_OPS( UIElementUserFlags );

enum class UIElementScriptFlags : uint8_t
{
    NONE              = 0,
    UPDATE            = ( 1u << 0 ),
    MOUSE_ENTER       = ( 1u << 1 ),
    MOUSE_LEAVE       = ( 1u << 2 ),
    MOUSE_BUTTON_DOWN = ( 1u << 3 ),
    MOUSE_BUTTON_UP   = ( 1u << 4 ),
};
PG_DEFINE_ENUM_OPS( UIElementScriptFlags );

enum class UIElementBlendMode : uint8_t
{
    OPAQUE   = 0,
    BLEND    = 1,
    ADDITIVE = 2,

    COUNT = 3
};

enum class UIElementType : uint8_t
{
    DEFAULT        = 0,
    MKDD_DIAG_TINT = 1,

    COUNT = 2
};

using UIElementHandle                           = uint16_t;
static constexpr UIElementHandle UI_NULL_HANDLE = UINT16_MAX;
static constexpr uint16_t UI_NO_SCRIPT_INDEX    = UINT16_MAX;

struct UIElement
{
    UIElementHandle parent;
    UIElementHandle prevSibling;
    UIElementHandle nextSibling;
    UIElementHandle firstChild;
    UIElementHandle lastChild;

    uint16_t scriptFunctionsIdx;

    UIElementType type               = UIElementType::DEFAULT;
    UIElementUserFlags userFlags     = UIElementUserFlags::VISIBLE;
    UIElementScriptFlags scriptFlags = UIElementScriptFlags::NONE;
    UIElementBlendMode blendMode     = UIElementBlendMode::OPAQUE;
    vec2 pos;        // normalized 0 - 1
    vec2 dimensions; // normalized 0 - 1
    vec4 tint;
    GfxImage* image = nullptr;

    UIElementHandle Handle() const;
};

constexpr int AAAAAA = sizeof( UIElement );

} // namespace PG::UI
