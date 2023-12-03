#include "ImageLib/image.hpp"
#include "core/low_discrepancy_sampling.hpp"
#include "renderer/brdf_functions.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"

using namespace PG;

// https://learnopengl.com/PBR/IBL/Specular-IBL
vec2 IntegrateBRDF( float NdotV, float perceptualRoughness )
{
    vec3 V;
    V.x = sqrt( 1.0f - NdotV * NdotV );
    V.y = 0.0;
    V.z = NdotV;

    float linearRoughness = perceptualRoughness * perceptualRoughness;

    float scale = 0.0;
    float bias  = 0.0;

    vec3 N( 0.0f, 0.0f, 1.0f );

    const uint32_t SAMPLE_COUNT = 1024u;
    for ( uint32_t i = 0u; i < SAMPLE_COUNT; ++i )
    {
        vec2 Xi = vec2( i / (float)SAMPLE_COUNT, Hammersley32( i ) );
        vec3 H  = ImportanceSampleGGX_D( Xi, N, linearRoughness );
        vec3 L  = Normalize( 2.0f * Dot( V, H ) * H - V );

        float NdotL = Max( L.z, 0.0f );
        float NdotH = Max( H.z, 0.0f );
        float VdotH = Max( Dot( V, H ), 0.0f );

        if ( NdotL > 0.0f )
        {
#if 0 // LearnOpenGL method
            float G = GeometrySmith_IBL( N, V, L, perceptualRoughness );
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow( 1.0f - VdotH, 5.0f );
            scale += (1.0f - Fc) * G_Vis;
            bias += Fc * G_Vis;

            // Should we be doing something like this, since we use PBR_FresnelSchlickRoughness not the regular PBR_FresnelSchlick?
            //    and if so, would that mean we need to introduce multiple brdfLUT's since due to the max( gloss, F0 ), so 1 brdfLUT per discrete F0 grayscale val?
            //bias += (Fc * (1.0f - roughness)) * G_Vis;
#else
            float V_pdf = V_SmithGGXCorrelated( NdotV, NdotL, linearRoughness ) * VdotH * NdotL / NdotH;
            float Fc    = pow( 1.0f - VdotH, 5.0f );
            scale += 4.0f * ( 1.0f - Fc ) * V_pdf;
            bias += 4.0f * Fc * V_pdf;
#endif
        }
    }
    scale /= (float)SAMPLE_COUNT;
    bias /= (float)SAMPLE_COUNT;

    return clamp( vec2( scale, bias ), vec2( 0 ), vec2( 1 ) );
}

int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    constexpr int LUT_RESOLUTION = 1024;
    FloatImage2D lut( LUT_RESOLUTION, LUT_RESOLUTION, 3 );

    #pragma omp parallel for
    for ( int row = 0; row < LUT_RESOLUTION; ++row )
    {
        for ( int col = 0; col < LUT_RESOLUTION; ++col )
        {
            vec2 color = IntegrateBRDF( ( col + 0.5f ) / (float)LUT_RESOLUTION, ( row + 0.5f ) / (float)LUT_RESOLUTION );
            lut.SetFromFloat4( LUT_RESOLUTION - row - 1, col, vec4( color, 0, 1 ) ); // outputting image with 0,0 as bottom left
        }
    }

    lut.Save( PG_ASSET_DIR "images/brdf_integrate_lut.exr", ImageSaveFlags::KEEP_FLOATS_AS_32_BIT );

    Logger_Shutdown();

    return 0;
}
