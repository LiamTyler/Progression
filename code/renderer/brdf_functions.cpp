#include "renderer/brdf_functions.hpp"
#include "shared/math.hpp"

using namespace glm;

namespace PG
{

vec3 ImportanceSampleGGX_D( vec2 Xi, vec3 N, float roughness )
{
    float a = roughness * roughness; // note: Epic games used square roughness to better match Disney's original pbr research
	
    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt( (1.0f - Xi.y) / (1.0f + (a*a - 1.0f) * Xi.y) );
    float sinTheta = sqrt( 1.0f - cosTheta * cosTheta );
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos( phi ) * sinTheta;
    H.y = sin( phi ) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs( N.z ) < 0.999f ? vec3( 0.0f, 0.0f, 1.0f ) : vec3( 1.0f, 0.0f, 0.0f );
    vec3 tangent   = normalize( cross( up, N ) );
    vec3 bitangent = cross( N, tangent );
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize( sampleVec );
}


float GeometrySchlickGGX_IBL( float NdotV, float roughness )
{
    // note that we use a different k for IBL vs Direct lighting
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}


float GeometrySmith_IBL( vec3 N, vec3 V, vec3 L, float roughness )
{
    float NdotV = std::max( dot( N, V ), 0.0f );
    float NdotL = std::max( dot( N, L ), 0.0f );
    float ggx2 = GeometrySchlickGGX_IBL( NdotV, roughness );
    float ggx1 = GeometrySchlickGGX_IBL( NdotL, roughness );

    return ggx1 * ggx2;
}


float GeometrySchlickGGX_Direct( float NdotV, float roughness )
{
    // note that we use a different k for IBL vs Direct lighting
    float a = roughness + 1.0f;
    float k = (a * a) / 8.0f;

    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}


float GeometrySmith_Direct( vec3 N, vec3 V, vec3 L, float roughness )
{
    float NdotV = std::max( dot( N, V ), 0.0f );
    float NdotL = std::max( dot( N, L ), 0.0f );
    float ggx2 = GeometrySchlickGGX_Direct( NdotV, roughness );
    float ggx1 = GeometrySchlickGGX_Direct( NdotL, roughness );

    return ggx1 * ggx2;
}

} // namespace PG