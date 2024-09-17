#ifndef __DEBUG_TEXT_H__
#define __DEBUG_TEXT_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct TextDrawData
{
    Vec4Buffer vertexBuffer;
    uint fontTexture;
    uint _pad;
};

END_GPU_DATA_NAMESPACE()

#endif // __DEBUG_TEXT_H__