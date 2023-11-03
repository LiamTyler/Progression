#include "tonemap.hpp"
#include "shared/logger.hpp"
#include <unordered_map>

namespace PT
{


TonemapOperator TonemapOperatorFromString( const std::string& name )
{
    std::unordered_map< std::string, TonemapOperator > map =
    {
        { "NONE",       TonemapOperator::NONE },
        { "REINHARD",   TonemapOperator::REINHARD },
        { "UNCHARTED2", TonemapOperator::UNCHARTED2 },
        { "ACES",       TonemapOperator::ACES },
    };

    auto it = map.find( name );
    if ( it == map.end() )
    {
        LOG_WARN( "Tonemap operator '%s' not found, using NONE instead!", name.c_str() );
        return TonemapOperator::NONE;
    }

    return it->second;
}


glm::vec3 NoTonemap( const glm::vec3& pixel )
{
    return pixel;
}


glm::vec3 ReinhardTonemap( const glm::vec3& pixel )
{
    return pixel / ( glm::vec3( 1.0f ) + pixel );
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

glm::vec3 Uncharted2Tonemap( const glm::vec3& pixel )
{
    glm::vec3 curr       = Uncharted2TonemapHelper( pixel );
    glm::vec3 whiteScale = 1.0f / Uncharted2TonemapHelper( glm::vec3( 11.2f ) );
    glm::vec3 color      = curr * whiteScale;
    return color;
}


// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
glm::vec3 AcesTonemap( const glm::vec3& x )
{
    constexpr float a = 2.51f;
    constexpr float b = 0.03f;
    constexpr float c = 2.43f;
    constexpr float d = 0.59f;
    constexpr float e = 0.14f;
    return glm::clamp( (x*(a*x+b)) / (x*(c*x+d)+e), glm::vec3( 0 ), glm::vec3( 1 ) );
}


TonemapFunc GetTonemapFunction( TonemapOperator op )
{
    static TonemapFunc tonemapFuncs[] =
    {
        NoTonemap,
        ReinhardTonemap,
        Uncharted2Tonemap,
        AcesTonemap,
    };

    return tonemapFuncs[static_cast<int>( op )];
}

} // namespace PT