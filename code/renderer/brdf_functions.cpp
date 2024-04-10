#include "renderer/brdf_functions.hpp"

namespace PG
{

vec3 FresnelSchlick( float NdotV, vec3 F0 ) { return F0 + ( vec3( 1.0f ) - F0 ) * pow( 1.0f - NdotV, 5.0f ); }

float GGX_D( float NdotH, float roughness )
{
    float a     = roughness * roughness;
    float a2    = a * a;
    float denom = ( NdotH * NdotH * ( a2 - 1.0f ) + 1.0f );

    return a2 / ( PI * denom * denom );
}

vec3 ImportanceSampleGGX_D( vec2 Xi, vec3 N, float linearRoughness )
{
    float a = linearRoughness;

    float phi      = 2.0f * PI * Xi.x;
    float cosTheta = sqrt( ( 1.0f - Xi.y ) / ( 1.0f + ( a * a - 1.0f ) * Xi.y ) );
    float sinTheta = sqrt( 1.0f - cosTheta * cosTheta );

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos( phi ) * sinTheta;
    H.y = sin( phi ) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs( N.z ) < 0.999f ? vec3( 0.0f, 0.0f, 1.0f ) : vec3( 1.0f, 0.0f, 0.0f );
    vec3 tangent   = Normalize( Cross( up, N ) );
    vec3 bitangent = Cross( N, tangent );

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return Normalize( sampleVec );
}

float GeometrySchlickGGX_IBL( float NdotV, float perceptualRoughness )
{
    // note that we use a different k for IBL vs Direct lighting
    float a = perceptualRoughness;
    float k = ( a * a ) / 2.0f;

    return NdotV / ( NdotV * ( 1.0f - k ) + k );
}

float GeometrySmith_IBL( vec3 N, vec3 V, vec3 L, float perceptualRoughness )
{
    float NdotV = Max( Dot( N, V ), 0.0f );
    float NdotL = Max( Dot( N, L ), 0.0f );
    float ggx2  = GeometrySchlickGGX_IBL( NdotV, perceptualRoughness );
    float ggx1  = GeometrySchlickGGX_IBL( NdotL, perceptualRoughness );

    return ggx1 * ggx2;
}

float GeometrySchlickGGX_Direct( float NdotV, float perceptualRoughness )
{
    // note that we use a different k for IBL vs Direct lighting
    float a = perceptualRoughness + 1.0f;
    float k = ( a * a ) / 8.0f;

    return NdotV / ( NdotV * ( 1.0f - k ) + k );
}

float GeometrySmith_Direct( vec3 N, vec3 V, vec3 L, float perceptualRoughness )
{
    float NdotV = Max( Dot( N, V ), 0.0f );
    float NdotL = Max( Dot( N, L ), 0.0f );
    float ggx2  = GeometrySchlickGGX_Direct( NdotV, perceptualRoughness );
    float ggx1  = GeometrySchlickGGX_Direct( NdotL, perceptualRoughness );

    return ggx1 * ggx2;
}

// https://google.github.io/filament/Filament.html#materialsystem/specularbrdf section 4.4.2
float V_SmithGGXCorrelated( float NoV, float NoL, float linearRoughness )
{
    float a2   = linearRoughness * linearRoughness;
    float GGXV = NoL * sqrt( NoV * NoV * ( 1.0f - a2 ) + a2 );
    float GGXL = NoV * sqrt( NoL * NoL * ( 1.0f - a2 ) + a2 );
    return 0.5f / ( GGXV + GGXL );
}

} // namespace PG
