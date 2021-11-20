#include "tonemap.hpp"

namespace PT
{

glm::vec3 GammaCorrect( const glm::vec3& pixel, float gamma )
{
    return glm::pow( pixel, glm::vec3( 1.0f ) / glm::vec3( gamma ) );
}

glm::vec3 PBRTGammaCorrect( const glm::vec3& pixel )
{
    glm::vec3 newPixel;
    for ( int i = 0; i < 3; ++i )
    {
        if ( pixel[i] <= 0.0031308f )
        {
            newPixel[i] = 12.92f * pixel[i];
        }
        else
        {
            newPixel[i] = 1.055f * std::pow( pixel[i], 1.f / 2.4f ) - 0.055f;
        }
    }
    
    return newPixel;
}

glm::vec3 ReinhardTonemap( const glm::vec3& pixel, float exposure )
{
    auto adjustedColor = exposure * pixel;
    return adjustedColor / ( glm::vec3( 1.0f ) + adjustedColor );
}

// source: http://filmicworlds.com/blog/filmic-tonemapping-operators/
static glm::vec3 Uncharted2TonemapHelper( const glm::vec3& x )
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

glm::vec3 Uncharted2Tonemap( const glm::vec3& pixel, float exposure )
{
    glm::vec3 curr       = Uncharted2TonemapHelper( exposure * pixel );
    glm::vec3 whiteScale = 1.0f / Uncharted2TonemapHelper( glm::vec3( 11.2f ) );
    glm::vec3 color      = curr * whiteScale;
    return color;
}
} // namespace PT