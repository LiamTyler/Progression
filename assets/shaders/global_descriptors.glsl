#ifndef __GLOBAL_DESCRIPTORS_GLSL__
#define __GLOBAL_DESCRIPTORS_GLSL__

#include "common.glsl"
#include "c_shared/bindless.h"
#include "c_shared/structs.h"

layout( set = 0, binding = PG_BINDLESS_READ_ONLY_TEXTURE_DSET_BINDING ) uniform readonly image2D readOnlyImages[];
layout( set = 0, binding = PG_BINDLESS_RW_TEXTURE_DSET_BINDING ) uniform image2D rwImages[];
layout( scalar, set = 0, binding = PG_SCENE_GLOBALS_DSET_BINDING ) uniform SceneGlobalsUBO
{
    SceneGlobals globals;
};
layout( scalar, set = 0, binding = PG_BINDLESS_BUFFER_DSET_BINDING ) readonly buffer BindlessBuffers
{
    uint64_t bindlessBuffers[];
};

DEFINE_BUFFER_REFERENCE( 64 ) TransformBuffer
{
    mat4 transforms[];
};

mat4 GetModelMatrix( uint objectIdx )
{
    TransformBuffer tBuffer = TransformBuffer( bindlessBuffers[globals.modelMatriciesIdx] );
    return tBuffer.transforms[objectIdx];
}

mat4 GetNormalMatrix( uint objectIdx )
{
    TransformBuffer tBuffer = TransformBuffer( bindlessBuffers[globals.normalMatriciesIdx] );
    return tBuffer.transforms[objectIdx];
}

#endif // #ifndef __GLOBAL_DESCRIPTORS_GLSL__