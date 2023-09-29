#include "ImageLib/image.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"

using namespace glm;

float RadicalInverse_VdC( uint32_t bits ) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float( bits ) * 2.3283064365386963e-10f; // / 0x100000000
}

vec2 Hammersley( uint i, uint N )
{
    return vec2( float( i ) / float( N ), RadicalInverse_VdC( i ) );
}

vec3 ImportanceSampleGGX( vec2 Xi, vec3 N, float roughness )
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
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize( sampleVec );
}

float GeometrySchlickGGX( float NdotV, float roughness )
{
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0f;

    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / denom;
}

float GeometrySmith( vec3 N, vec3 V, vec3 L, float roughness )
{
    float NdotV = max( dot(N, V), 0.0f );
    float NdotL = max( dot(N, L), 0.0f );
    float ggx2 = GeometrySchlickGGX( NdotV, roughness );
    float ggx1 = GeometrySchlickGGX( NdotL, roughness );

    return ggx1 * ggx2;
}

// https://learnopengl.com/PBR/IBL/Specular-IBL
vec2 IntegrateBRDF( float NdotV, float roughness )
{
    vec3 V;
    V.x = sqrt( 1.0f - NdotV * NdotV );
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N( 0.0f, 0.0f, 1.0f );

    const uint32_t SAMPLE_COUNT = 1024u;
    for ( uint32_t i = 0u; i < SAMPLE_COUNT; ++i )
    {
        vec2 Xi = Hammersley( i, SAMPLE_COUNT );
        vec3 H  = ImportanceSampleGGX( Xi, N, roughness );
        vec3 L  = normalize( 2.0f * dot(V, H) * H - V );

        float NdotL = max( L.z, 0.0f );
        float NdotH = max( H.z, 0.0f );
        float VdotH = max( dot( V, H ), 0.0f );

        if ( NdotL > 0.0f )
        {
            float G = GeometrySmith( N, V, L, roughness );
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow( 1.0f - VdotH, 5.0f );

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= (float)SAMPLE_COUNT;
    B /= (float)SAMPLE_COUNT;

    return vec2( A, B );
}

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    constexpr uint32_t LUT_RESOLUTION = 512;
    FloatImage2D lut( LUT_RESOLUTION, LUT_RESOLUTION, 3 );
    
    #pragma omp parallel for
    for ( int row = 0; row < LUT_RESOLUTION; ++row )
    {
        for ( int col = 0; col < LUT_RESOLUTION; ++col )
        {
            vec2 color = IntegrateBRDF( (col + 0.5f) / (float)LUT_RESOLUTION, (row + 0.5f) / (float)LUT_RESOLUTION );
            lut.SetFromFloat4( LUT_RESOLUTION - row - 1, col, vec4( color, 0, 1 ) ); // outputting image with 0,0 as bottom left
        }
    }

    lut.Save( PG_ASSET_DIR "images/brdf_integrate_lut.png" );

    Logger_Shutdown();

    return 0;
}