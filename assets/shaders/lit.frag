#version 450

#include "global_descriptors.glsl"
#include "lib/packing.glsl"

layout (location = 0) in PerVertexData
{
    vec3 normal;
    vec4 tangentAndSign;
    vec2 uv;
#if IS_DEBUG_SHADER
    flat uint meshletIdx;
#endif // #if IS_DEBUG_SHADER
} fragInput;

layout (location = 0) out vec4 color;

struct Material
{
    vec3 albedo;
    float metalness;
    vec3 N;
    float roughness;
    vec3 emissive;
    vec3 F0;
};

void GetAlbedoMetalness( const vec2 uv, out vec3 albedo, out float metalness )
{
    albedo = vec3( 0, 1, 0 );
    metalness = 0;
}

void GetNormalRoughness( const vec2 uv, const vec3 geomNormal, out vec3 N, out float roughness )
{
    N = geomNormal;
    roughness = 1.0f;
}

vec3 GetEmissive( const vec2 uv )
{
    return vec3( 0 );
}

Material GetMaterial( const vec2 uv )
{
    Material m;
    GetAlbedoMetalness( uv, m.albedo, m.metalness );
    GetNormalRoughness( uv, fragInput.normal, m.N, m.roughness );
    m.emissive = GetEmissive( uv );
    m.F0 = mix( vec3( 0.04f ), m.albedo, m.metalness );

    return m;
}

#if IS_DEBUG_SHADER
#include "c_shared/dvar_defines.h"
#include "lib/debug_coloring.glsl"
#include "lib/debug_wireframe.glsl"

void Debug_Geometry( inout vec4 outColor )
{
    uint r_geometryViz = globals.r_geometryViz;
    if ( r_geometryViz == PG_DEBUG_GEOM_UV )
    {
        outColor.rgb = vec3( fragInput.uv, 0 );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( normalize( fragInput.normal ) );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_TANGENT )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( normalize( fragInput.tangentAndSign.xyz ) );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_BITANGENT )
    {
        vec3 normal = normalize( fragInput.normal );
        vec3 tangent = normalize( fragInput.tangentAndSign.xyz );
        vec3 bitangent = cross( normal, tangent ) * fragInput.tangentAndSign.w;
        outColor.rgb = Vec3ToUnorm_Clamped( bitangent );
    }
}

void Debug_Material( const Material m, inout vec4 outColor )
{
    uint r_materialViz = globals.r_materialViz;
    if ( r_materialViz == PG_DEBUG_MTL_ALBEDO )
    {
        outColor.rgb = vec3( 0.5, 0.5, 0.5 );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_NORMAL )
    {
        outColor.rgb = Vec3ToUnorm_Clamped( m.N );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_ROUGHNESS )
    {
        outColor.rgb = vec3( m.roughness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_METALNESS )
    {
        outColor.rgb = vec3( m.metalness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_EMISSIVE )
    {
        outColor.rgb = vec3( m.emissive );
    }
}

#endif // #if IS_DEBUG_SHADER

void main()
{    
    Material m = GetMaterial( vec2( 0 ) );
    color = vec4( m.albedo, 1 );
    
#if IS_DEBUG_SHADER
    Debug_Geometry( color );
    Debug_Material( m, color );
    if ( IsMeshletVizEnabled() )
    {
        color = Debug_IndexToColorVec4( fragInput.meshletIdx );
    }
    if ( IsWireframeEnabled() )
    {
        color = ProcessStandardWireframe( color );
    }
#endif // #if IS_DEBUG_SHADER
}
