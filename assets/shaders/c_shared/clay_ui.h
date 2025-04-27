#ifndef __CLAY_UI_H__
#define __CLAY_UI_H__

#include "c_shared/defines.h"

#ifndef PG_SHADER_CODE

namespace GpuData
{
#endif // #ifndef PG_SHADER_CODE

struct UIRectElementData
{
    mat4 projMatrix;
    vec4 aabb; // x = posX, y = posY, z = width, w = height
    vec4 color;
    uint textureIndex;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData

#endif // #ifndef PG_SHADER_CODE

#endif // #ifndef __CLAY_UI_H__