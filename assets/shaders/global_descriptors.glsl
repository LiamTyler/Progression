#ifndef __GLOBAL_DESCRIPTORS_GLSL__
#define __GLOBAL_DESCRIPTORS_GLSL__

#include "common.glsl"
#include "c_shared/bindless.h"
#include "c_shared/material.h"
#include "c_shared/scene_globals.h"

layout( set = 0, binding = PG_BINDLESS_SAMPLED_TEXTURE_DSET_BINDING ) uniform texture2D Textures2D[];
layout( set = 0, binding = PG_BINDLESS_READ_ONLY_IMAGE_DSET_BINDING ) uniform readonly image2D RImages2D[];
layout( set = 0, binding = PG_BINDLESS_RW_IMAGE_DSET_BINDING ) uniform image2D RWImages2D[];
layout( scalar, set = 0, binding = PG_SCENE_GLOBALS_DSET_BINDING ) uniform SceneGlobalsUBO
{
    SceneGlobals globals;
};
layout( scalar, set = 0, binding = PG_BINDLESS_BUFFER_DSET_BINDING ) readonly buffer BindlessBuffers
{
    uint64_t bindlessBuffers[];
};
layout( scalar, set = 0, binding = PG_BINDLESS_MATERIALS_DSET_BINDING ) readonly buffer Materials
{
    Material materials[];
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

Material GetMaterial( uint matIdx )
{
    return materials[matIdx];
}

#endif // #ifndef __GLOBAL_DESCRIPTORS_GLSL__