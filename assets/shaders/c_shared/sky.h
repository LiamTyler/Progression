#ifndef __CULL_DATA_H__
#define __CULL_DATA_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct SkyDrawData
{
    mat4 VP;
    Vec3Buffer vertexBuffer;
    uint skyTextureIndex;
    uint _pad;
};

END_GPU_DATA_NAMESPACE()

#endif // __CULL_DATA_H__