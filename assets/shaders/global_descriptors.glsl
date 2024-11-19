#ifndef __GLOBAL_DESCRIPTORS_GLSL__
#define __GLOBAL_DESCRIPTORS_GLSL__

#include "common.glsl"
#include "c_shared/bindless.h"
#include "c_shared/lights.h"
#include "c_shared/material.h"
#include "c_shared/scene_globals.h"

layout( set = 0, binding = PG_BINDLESS_SAMPLED_TEXTURE_DSET_BINDING ) uniform texture2D Textures2D[];
layout( set = 0, binding = PG_BINDLESS_STORAGE_IMAGE_DSET_BINDING ) uniform restrict readonly image2D RImages2D[];
layout( set = 0, binding = PG_BINDLESS_STORAGE_IMAGE_DSET_BINDING ) uniform restrict image2D RWImages2D[];
layout( scalar, set = 0, binding = PG_SCENE_GLOBALS_DSET_BINDING ) uniform restrict SceneGlobalsUBO
{
    SceneGlobals globals;
};
layout( scalar, set = 0, binding = PG_BINDLESS_BUFFER_DSET_BINDING ) restrict readonly buffer BindlessBuffers
{
    uint64_t bindlessBuffers[];
};
layout( scalar, set = 0, binding = PG_BINDLESS_MATERIALS_DSET_BINDING ) restrict readonly buffer Materials
{
    PackedMaterial packedMaterials[];
};

layout( set = 0, binding = PG_BINDLESS_SAMPLERS_DSET_BINDING ) uniform sampler samplers[];

// Note: keep in sync with sampler.hpp!
#define SAMPLER_NEAREST                 0
#define SAMPLER_NEAREST_WRAP_U          1
#define SAMPLER_NEAREST_WRAP_V          2
#define SAMPLER_NEAREST_WRAP_U_WRAP_V   3
#define SAMPLER_BILINEAR                4
#define SAMPLER_BILINEAR_WRAP_U         5
#define SAMPLER_BILINEAR_WRAP_V         6
#define SAMPLER_BILINEAR_WRAP_U_WRAP_V  7
#define SAMPLER_TRILINEAR               8
#define SAMPLER_TRILINEAR_WRAP_U        9
#define SAMPLER_TRILINEAR_WRAP_V        10
#define SAMPLER_TRILINEAR_WRAP_U_WRAP_V 11

vec4 SampleTexture2D( uint texIndex, uint samplerIndex, vec2 uv )
{
    return texture( nonuniformEXT( sampler2D( Textures2D[texIndex], samplers[samplerIndex] ) ), uv );
}

vec4 SampleTexture2D_Uniform( uint texIndex, uint samplerIndex, vec2 uv )
{
    return texture( sampler2D( Textures2D[texIndex], samplers[samplerIndex] ), uv );
}

DEFINE_BUFFER_REFERENCE( 64 ) TransformBuffer
{
    mat4 transforms[];
};

mat4 GetModelMatrix( uint modelIdx )
{
    TransformBuffer tBuffer = TransformBuffer( globals.modelMatriciesBufferAddress );
    mat4 M = tBuffer.transforms[modelIdx];
    M[0].w = 0;
    M[1].w = 0;
    M[2].w = 0;
    return M;
}

// instead of using the inverse transpose of the model matrix for normals,
// you can actually just scale the normal by 1.0/(scale*scale) before multiplying by the model matrix
// https://lxjk.github.io/2017/10/01/Stop-Using-Normal-Matrix.html
mat4 GetModelMatrixAndNormalScale( uint modelIdx, out vec3 normalScale )
{
    TransformBuffer tBuffer = TransformBuffer( globals.modelMatriciesBufferAddress );
    mat4 M = tBuffer.transforms[modelIdx];
    normalScale.x = M[0].w;
    normalScale.y = M[1].w;
    normalScale.z = M[2].w;
    M[0].w = 0;
    M[1].w = 0;
    M[2].w = 0;
    return M;
}

PackedMaterial GetPackedMaterial( uint matIdx )
{
    return packedMaterials[matIdx];
}

#endif // #ifndef __GLOBAL_DESCRIPTORS_GLSL__