#version 450

#include "global_descriptors.glsl"
#include "lib/lights.glsl"
#include "lib/pbr.glsl"
#include "lib/packing.glsl"
#include "c_shared/lights.h"

layout (location = 0) in PerVertexData
{
    vec3 worldPos;
    vec3 normal;
    vec4 tangentAndSign;
    vec2 uv;
    flat uint materialIdx;
#if IS_DEBUG_SHADER
    flat uint meshletIdx;
#endif // #if IS_DEBUG_SHADER
} fragInput;

layout (location = 0) out vec4 fragOutColor;

void GetAlbedoMetalness( const PackedMaterial mat, const vec2 uv, out vec3 albedo, out float metalness )
{
    albedo = mat.albedoTint.rgb;
    metalness = mat.metalnessTint;

    if ( mat.albedoMetalnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 albedoMetalnessSample = SampleTexture2D( mat.albedoMetalnessMapIndex, SAMPLER_TRILINEAR_WRAP_U_WRAP_V, uv );
        albedo *= albedoMetalnessSample.rgb;
        metalness *= albedoMetalnessSample.a;
    }
}

void GetNormalRoughness( const PackedMaterial mat, const vec2 uv, out vec3 N, out float perceptualRoughness )
{
    N = normalize( fragInput.normal );
    perceptualRoughness = mat.roughnessTint;

    if ( mat.normalRoughnessMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 normalRoughnessSample = SampleTexture2D( mat.normalRoughnessMapIndex, SAMPLER_TRILINEAR_WRAP_U_WRAP_V, uv );
        vec3 nmv = UnpackNormalMapVal( normalRoughnessSample.rgb );
        vec3 T = normalize( fragInput.tangentAndSign.xyz );
        vec3 B = cross( N, T ) * fragInput.tangentAndSign.w;
        N = normalize( T * nmv.x + B * nmv.y + N * nmv.z );

        perceptualRoughness *= normalRoughnessSample.a;
    }
}

vec3 GetEmissive( const PackedMaterial mat, const vec2 uv )
{
    vec3 emissive = mat.emissiveTint;
    if ( mat.emissiveMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec3 emissiveSample = SampleTexture2D( mat.emissiveMapIndex, SAMPLER_TRILINEAR_WRAP_U_WRAP_V, uv ).rgb;
        emissive *= emissiveSample;
    }

    return emissive;
}

struct ShaderMaterial
{
    vec3 albedo;
    float metalness;
    vec3 N;
    float perceptualRoughness;
    vec3 emissive;
    vec3 F0;
};

ShaderMaterial GetShaderMaterial()
{
    PackedMaterial m = GetPackedMaterial( fragInput.materialIdx );
    ShaderMaterial sm;
    GetAlbedoMetalness( m, fragInput.uv, sm.albedo, sm.metalness );
    GetNormalRoughness( m, fragInput.uv, sm.N, sm.perceptualRoughness );
    sm.emissive = GetEmissive( m, fragInput.uv );
    sm.F0 = mix( vec3( 0.04f ), sm.albedo, sm.metalness );

    return sm;
}

struct LightingResults
{
    vec3 direct;
    vec3 indirect;
    vec3 emissive;
};

DEFINE_STANDARD_BUFFER_REFERENCE( 64, LightBuffer, PackedLight );

void DirectLighting( const ShaderMaterial m, vec3 pos, vec3 V, float NdotV, out vec3 Lo )
{
    LightBuffer lights = LightBuffer( globals.lightBuffer );

    // https://google.github.io/filament/Filament.html#materialsystem/parameterization/remapping
    float roughness = m.perceptualRoughness * m.perceptualRoughness;
    vec3 diffuse = BRDF_Lambertian( m.albedo ) * (1.0f - m.metalness);
    Lo = vec3( 0 );

    const uint numLights = globals.numLights.x;
    for ( uint lightIdx = 0; lightIdx < numLights; ++lightIdx )
    {
        const PackedLight packedL = lights.data[lightIdx];
        vec3 radiance = packedL.colorAndType.rgb;
        const uint lightType = UnpackLightType( packedL );
        vec3 L = vec3( 0 );
        switch ( lightType )
        {
            case LIGHT_TYPE_POINT:
            {
                PointLight l = UnpackPointLight( packedL );
                radiance *= PointLightAttenuation( pos, l );
                L = normalize( l.position - pos );
                break;
            }
            case LIGHT_TYPE_SPOT:
            {
                SpotLight l = UnpackSpotLight( packedL );
                radiance *= SpotLightAttenuation( pos, l );
                L = normalize( l.position - pos );
                break;
            }
            case LIGHT_TYPE_DIRECTIONAL:
            {
                DirectionalLight l = UnpackDirectionalLight( packedL );
                radiance = l.color;
                L = -l.direction;
                break;
            }
        }

        vec3 H = normalize( V + L );
        float NdotH = max( dot( m.N, H ), 0.0f );
        float NdotL = max( dot( m.N, L ), 0.0f );
        float VdotH = max( dot( V, H ), 0.0f );

        float D = PBR_D_GGX( NdotH, roughness );
        float V = V_SmithGGXCorrelated( NdotV, NdotL, m.perceptualRoughness );
        vec3 F = PBR_FresnelSchlick( VdotH, m.F0 );
        
        vec3 Fr = D * V * F;
        vec3 Fd = diffuse * (1.0f - F);

        Lo += (Fd + Fr) * radiance * NdotL;
    }
}

void IndirectLighting( const ShaderMaterial m, out vec3 Lo )
{
    vec3 diffuse = BRDF_Lambertian( m.albedo ) * (1.0f - m.metalness);
    Lo = diffuse * globals.ambientColor.xyz;
}

void EmissiveLighting( const ShaderMaterial m, out vec3 Lo )
{
    Lo = m.emissive;
}

#if IS_DEBUG_SHADER
#include "c_shared/dvar_defines.h"
#include "lib/debug_coloring.glsl"
#include "lib/debug_wireframe.glsl"

void Debug_Geometry( inout vec4 color )
{
    uint r_geometryViz = globals.r_geometryViz;
    if ( r_geometryViz == PG_DEBUG_GEOM_UV )
    {
        color.rgb = vec3( fragInput.uv, 0 );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_NORMAL )
    {
        color.rgb = Vec3ToUnorm_Clamped( normalize( fragInput.normal ) );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_TANGENT )
    {
        color.rgb = Vec3ToUnorm_Clamped( normalize( fragInput.tangentAndSign.xyz ) );
    }
    else if ( r_geometryViz == PG_DEBUG_GEOM_BITANGENT )
    {
        vec3 normal = normalize( fragInput.normal );
        vec3 tangent = normalize( fragInput.tangentAndSign.xyz );
        vec3 bitangent = cross( normal, tangent ) * fragInput.tangentAndSign.w;
        color.rgb = Vec3ToUnorm_Clamped( bitangent );
    }
}

void Debug_Material( const ShaderMaterial m, inout vec4 color )
{
    uint r_materialViz = globals.r_materialViz;
    if ( r_materialViz == PG_DEBUG_MTL_ALBEDO )
    {
        color.rgb = m.albedo;
    }
    else if ( r_materialViz == PG_DEBUG_MTL_NORMAL )
    {
        color.rgb = Vec3ToUnorm_Clamped( m.N );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_ROUGHNESS )
    {
        color.rgb = vec3( m.perceptualRoughness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_METALNESS )
    {
        color.rgb = vec3( m.metalness );
    }
    else if ( r_materialViz == PG_DEBUG_MTL_EMISSIVE )
    {
        color.rgb = vec3( m.emissive );
    }
}

void Debug_Lighting( const LightingResults lighting, inout vec4 color )
{
    uint r_lightingViz = globals.r_lightingViz;
    if ( r_lightingViz == PG_DEBUG_LIGHTING_DIRECT )
    {
        color.rgb = lighting.direct;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_INDIRECT )
    {
        color.rgb = lighting.indirect;
    }
    else if ( r_lightingViz == PG_DEBUG_LIGHTING_EMISSIVE )
    {
        color.rgb = lighting.emissive;
    }
}

#endif // #if IS_DEBUG_SHADER

void main()
{
    ShaderMaterial mat = GetShaderMaterial();

    vec3 V = normalize( globals.cameraPos.xyz - fragInput.worldPos );
    float NdotV = clamp( dot( mat.N, V ), 1e-5f, 1.0f );

    LightingResults lighting;
    DirectLighting( mat, fragInput.worldPos, V, NdotV, lighting.direct );
    IndirectLighting( mat, lighting.indirect );
    EmissiveLighting( mat, lighting.emissive );

    vec3 Lo = vec3( 0 );
    Lo += lighting.direct;
    Lo += lighting.indirect;
    Lo += lighting.emissive;
    fragOutColor = vec4( Lo, 1 );
    
#if IS_DEBUG_SHADER
    Debug_Geometry( fragOutColor );
    Debug_Lighting( lighting, fragOutColor );
    Debug_Material( mat, fragOutColor );
    if ( IsMeshletVizEnabled() )
    {
        fragOutColor = Debug_IndexToColorVec4( fragInput.meshletIdx );
    }
    if ( IsWireframeEnabled() )
    {
        fragOutColor = ProcessStandardWireframe( fragOutColor );
    }
#endif // #if IS_DEBUG_SHADER
}
