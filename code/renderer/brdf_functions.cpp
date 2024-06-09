#include "renderer/brdf_functions.hpp"

namespace PG
{

vec3 FresnelSchlick( f32 NdotV, vec3 F0 ) { return F0 + ( vec3( 1.0f ) - F0 ) * powf( 1.0f - NdotV, 5.0f ); }

f32 GGX_D( f32 NdotH, f32 roughness )
{
    f32 a     = roughness * roughness;
    f32 a2    = a * a;
    f32 denom = ( NdotH * NdotH * ( a2 - 1.0f ) + 1.0f );

    return a2 / ( PI * denom * denom );
}

vec3 ImportanceSampleGGX_D( vec2 Xi, vec3 N, f32 linearRoughness )
{
    f32 a = linearRoughness;

    f32 phi      = 2.0f * PI * Xi.x;
    f32 cosTheta = sqrt( ( 1.0f - Xi.y ) / ( 1.0f + ( a * a - 1.0f ) * Xi.y ) );
    f32 sinTheta = sqrt( 1.0f - cosTheta * cosTheta );

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

f32 GeometrySchlickGGX_IBL( f32 NdotV, f32 perceptualRoughness )
{
    // note that we use a different k for IBL vs Direct lighting
    f32 a = perceptualRoughness;
    f32 k = ( a * a ) / 2.0f;

    return NdotV / ( NdotV * ( 1.0f - k ) + k );
}

f32 GeometrySmith_IBL( vec3 N, vec3 V, vec3 L, f32 perceptualRoughness )
{
    f32 NdotV = Max( Dot( N, V ), 0.0f );
    f32 NdotL = Max( Dot( N, L ), 0.0f );
    f32 ggx2  = GeometrySchlickGGX_IBL( NdotV, perceptualRoughness );
    f32 ggx1  = GeometrySchlickGGX_IBL( NdotL, perceptualRoughness );

    return ggx1 * ggx2;
}

f32 GeometrySchlickGGX_Direct( f32 NdotV, f32 perceptualRoughness )
{
    // note that we use a different k for IBL vs Direct lighting
    f32 a = perceptualRoughness + 1.0f;
    f32 k = ( a * a ) / 8.0f;

    return NdotV / ( NdotV * ( 1.0f - k ) + k );
}

f32 GeometrySmith_Direct( vec3 N, vec3 V, vec3 L, f32 perceptualRoughness )
{
    f32 NdotV = Max( Dot( N, V ), 0.0f );
    f32 NdotL = Max( Dot( N, L ), 0.0f );
    f32 ggx2  = GeometrySchlickGGX_Direct( NdotV, perceptualRoughness );
    f32 ggx1  = GeometrySchlickGGX_Direct( NdotL, perceptualRoughness );

    return ggx1 * ggx2;
}

// https://google.github.io/filament/Filament.html#materialsystem/specularbrdf section 4.4.2
f32 V_SmithGGXCorrelated( f32 NoV, f32 NoL, f32 linearRoughness )
{
    f32 a2   = linearRoughness * linearRoughness;
    f32 GGXV = NoL * sqrt( NoV * NoV * ( 1.0f - a2 ) + a2 );
    f32 GGXL = NoV * sqrt( NoL * NoL * ( 1.0f - a2 ) + a2 );
    return 0.5f / ( GGXV + GGXL );
}

} // namespace PG
