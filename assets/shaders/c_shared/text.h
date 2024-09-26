#ifndef __TEXT_H__
#define __TEXT_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct TextDrawData
{
    Vec2Buffer vertexBuffer;
    uint sdfFontAtlasTex;
    uint _pad;
    vec2 invScale;
};

END_GPU_DATA_NAMESPACE()

#endif // __TEXT_H__