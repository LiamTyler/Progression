#ifndef __TEXT_H__
#define __TEXT_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct TextDrawData
{
    mat4 projMatrix;
    Vec2Buffer vertexBuffer;
    uint sdfFontAtlasTex;
    uint packedColor;
    float unitRange;
    uint _pad;
    vec2 unitRange3D;
};

END_GPU_DATA_NAMESPACE()

#endif // __TEXT_H__