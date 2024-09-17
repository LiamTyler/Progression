#ifndef __SKY_H__
#define __SKY_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct SkyDrawData
{
    mat4 VP;
    Vec3Buffer vertexBuffer;
    vec3 scaleFactor;
    uint skyTextureIndex;
//#if IS_DEBUG_SHADER
    uint r_skyboxViz;
    uint _pad;
//#endif // #if IS_DEBUG_SHADER
};

END_GPU_DATA_NAMESPACE()

#endif // __SKY_H__