#version 450

#extension GL_EXT_mesh_shader : require

#include "c_shared/cull_data.h"
#include "c_shared/mesh_shading_defines.h"
#include "c_shared/model.h"
#include "global_descriptors.glsl"
#include "lib/oct_encoding.glsl"

DEFINE_STANDARD_BUFFER_REFERENCE( 32, MeshletBuffer, Meshlet );
DEFINE_STANDARD_BUFFER_REFERENCE( 4, MeshDataBuffer, PerObjectData );
DEFINE_STANDARD_BUFFER_REFERENCE( 4, MeshletDrawCommandBuffer, MeshletDrawCommand_MDI );

#extension GL_ARB_shader_draw_parameters : enable
layout( scalar, push_constant ) uniform Registers
{
    MeshDataBuffer meshDataBuffer;
    MeshletDrawCommand_MDI indirectMeshletDrawBuffer;
};


Meshlet GetMeshlet( uint meshletID, uint baseBuffer )
{
    MeshletBuffer mBuffer = MeshletBuffer( bindlessBuffers[baseBuffer + MESH_BUFFER_MESHLETS] );
    return mBuffer.data[meshletID];
}

vec3 GetPos32( const Meshlet meshlet, uint baseBuffer, uint index )
{
    uint64_t ptr    = bindlessBuffers[baseBuffer + MESH_BUFFER_POSITIONS];
    Vec3Buffer buff = Vec3Buffer( ptr );
    return buff.data[index];
}

vec3 GetNormal( const Meshlet meshlet, uint baseBuffer, uint index )
{
    uint64_t ptr = bindlessBuffers[baseBuffer + MESH_BUFFER_NORMALS];
    Vec3Buffer buff = Vec3Buffer( ptr );
    return buff.data[index];
}

vec2 GetUV( const Meshlet meshlet, uint baseBuffer, uint index )
{
    uint64_t ptr = bindlessBuffers[baseBuffer + MESH_BUFFER_UVS];
    if ( ptr == 0 )
        return vec2( 0 );

    vec2 uv;
    Vec2Buffer buff = Vec2Buffer( ptr );
    uv              = buff.data[index];
    return uv;
}

vec4 GetTangentAndSign( const Meshlet meshlet, uint baseBuffer, mat3 modelMatrix, uint index )
{
    uint64_t ptr = bindlessBuffers[baseBuffer + MESH_BUFFER_TANGENTS];
    if ( ptr == 0 )
        return vec4( 0 );

    Vec4Buffer buff     = Vec4Buffer( ptr );
    vec4 tangentAndSign = buff.data[index];
    vec3 worldTangent   = modelMatrix * tangentAndSign.xyz;
    return vec4( worldTangent, tangentAndSign.w );
}


layout( location = 0 ) out PerVertexData
{
    vec3 worldPos;
    vec3 normal;
    vec4 tangentAndSign;
    vec2 uv;
    flat uint materialIdx;
#if IS_DEBUG_SHADER
    flat uint meshletIdx;
#endif // #if IS_DEBUG_SHADER
} v_out;

void main()
{
    const uint meshIndex = indirectMeshletDrawBuffer.data[gl_DrawIDARB].meshIndex;
    const uint meshletID = indirectMeshletDrawBuffer.data[gl_DrawIDARB].meshletIndex;
    const PerObjectData objectData = meshDataBuffer.data[meshIndex];

    const uint baseBuffer = objectData.bindlessRangeStart;
    Meshlet meshlet       = GetMeshlet( meshletID, baseBuffer );

    vec3 normalScale;
    mat4 modelMatrix = GetModelMatrixAndNormalScale( objectData.modelIdx, normalScale );

    const uint localVIdx = gl_VertexIndex;
    const uint globalVIdx = meshlet.vertexOffset + localVIdx;

    vec4 localPos  = vec4( GetPos32( meshlet, baseBuffer, globalVIdx ), 1 );
    vec4 worldPos  = modelMatrix * localPos;
    v_out.worldPos = worldPos.xyz;
    gl_Position    = globals.VP * worldPos;

    vec3 localNormal = GetNormal( meshlet, baseBuffer, globalVIdx );
    localNormal *= normalScale;
    v_out.normal = mat3( modelMatrix ) * localNormal.xyz;

    v_out.uv = GetUV( meshlet, baseBuffer, globalVIdx );

    vec4 tangentAndSign = GetTangentAndSign( meshlet, baseBuffer, mat3( modelMatrix ), globalVIdx );
    v_out.tangentAndSign = tangentAndSign;

    v_out.materialIdx = objectData.materialIdx;
#if IS_DEBUG_SHADER
    v_out.meshletIdx = meshletID;
#endif // #if IS_DEBUG_SHADER
}
