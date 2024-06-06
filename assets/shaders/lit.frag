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

layout( std430, push_constant ) uniform Registers
{
    layout( offset = 8 ) uint materialIdx;
};

layout (location = 0) out vec4 color;

//layout (set = 2, binding = 0) uniform sampler mySampler;

vec4 SampleTexture2D( uint texIndex, vec2 uv )
{
    return vec4( 0 ); //return texture( sampler2D( Textures2D[texIndex], mySampler ), uv );
}

void GetAlbedoMetalness( const Material mat, const vec2 uv, out vec3 albedo, out float metalness )
{
    albedo = mat.albedoTint.rgb;
    metalness = mat.metalnessTint;

    if ( mat.albedoMetalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 albedoMetalnessSample = SampleTexture2D( mat.albedoMetalnessMapIndex, uv );
        albedo *= albedoMetalnessSample.rgb;
        metalness *= albedoMetalnessSample.a;
    }
}

void GetNormalRoughness( const Material mat, const vec2 uv, out vec3 N, out float roughness )
{
    N = normalize( fragInput.normal );
    roughness = mat.roughnessTint;

    if ( mat.normalRoughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 normalRoughnessSample = SampleTexture2D( mat.normalRoughnessMapIndex, uv );
        vec3 nmv = UnpackNormalMapVal( normalRoughnessSample.rgb );
        vec3 T = normalize( fragInput.tangentAndSign.xyz );
        vec3 B = cross( N, T ) * fragInput.tangentAndSign.w;
        N = normalize( T * nmv.x + B * nmv.y + N * nmv.z );

        roughness *= normalRoughnessSample.a;
    }
}

vec3 GetEmissive( const Material mat, const vec2 uv )
{
    vec3 emissive = mat.emissiveTint;
    if ( mat.emissiveMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec3 emissiveSample = SampleTexture2D( mat.emissiveMapIndex, uv ).rgb;
        emissive *= emissiveSample;
    }

    return emissive;
}

struct ShaderMaterial
{
    vec3 albedo;
    float metalness;
    vec3 N;
    float roughness;
    vec3 emissive;
    vec3 F0;
};

ShaderMaterial GetShaderMaterial()
{
    Material m = GetMaterial( materialIdx );
    ShaderMaterial sm;
    GetAlbedoMetalness( m, fragInput.uv, sm.albedo, sm.metalness );
    GetNormalRoughness( m, fragInput.uv, sm.N, sm.roughness );
    sm.emissive = GetEmissive( m, fragInput.uv );
    sm.F0 = mix( vec3( 0.04f ), sm.albedo, sm.metalness );

    return sm;
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

void Debug_Material( const ShaderMaterial m, inout vec4 outColor )
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
    ShaderMaterial mat = GetShaderMaterial();
    color = vec4( mat.albedo, 1 );
    
#if IS_DEBUG_SHADER
    Debug_Geometry( color );
    Debug_Material( mat, color );
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
