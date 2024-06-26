#ifndef __UI_H__
#define __UI_H__

#include "c_shared/defines.h"

// keep in sync with UIElementUserFlags!
#define UI_FLAG_APPLY_TONEMAPPING (1 << 1)

#define UI_ELEM_TYPE_DEFAULT 0
#define UI_ELEM_TYPE_MKDD_DIAG_TINT 1
#define UI_ELEM_TYPE_COUNT 2

#ifndef PG_SHADER_CODE
static_assert( Underlying( ::PG::UI::UIElementType::COUNT ) == UI_ELEM_TYPE_COUNT );

namespace GpuData
{
#endif // #ifndef PG_SHADER_CODE

struct UIElementData
{
    uint flags;
    uint type;
	uint packedTint;
	uint textureIndex;
	vec2 pos;
	vec2 dimensions;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData

#endif // #ifndef PG_SHADER_CODE

#endif // #ifndef __UI_H__