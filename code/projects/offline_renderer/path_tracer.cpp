#include "path_tracer.hpp"
#include "anti_aliasing.hpp"
#include "core/time.hpp"
#include "glm/ext.hpp"
#include "sampling.hpp"
#include "shared/color_spaces.hpp"
#include "shared/core_defines.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include "tonemap.hpp"
#include <algorithm>
#include <atomic>
#include <fstream>

#define PROGRESS_BAR_STR "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
#define PROGRESS_BAR_WIDTH 60
#define EPSILON 0.00001f

using namespace PG;

namespace PT
{

vec3 EstimateSingleDirect( Light* light, const IntersectionData& hitData, Scene* scene, Random::RNG& rng, const BRDF& brdf )
{
    Interaction it{ hitData.position, hitData.normal };
    vec3 wi;
    f32 lightPdf;

    // get incoming radiance, and how likely it was to sample that direction on the light
    vec3 Li = light->Sample_Li( it, wi, scene, rng, lightPdf );
    if ( lightPdf == 0 )
    {
        return vec3( 0 );
    }

    return brdf.F( hitData.wo, wi ) * Li * AbsDot( hitData.normal, wi ) / lightPdf;
}

vec3 LDirect( const IntersectionData& hitData, Scene* scene, Random::RNG& rng, const BRDF& brdf )
{
    vec3 L( 0 );
    for ( const auto& light : scene->lights )
    {
        vec3 Ld( 0 );
        for ( i32 i = 0; i < light->nSamples; ++i )
        {
            Ld += EstimateSingleDirect( light, hitData, scene, rng, brdf );
        }
        L += Ld / (f32)light->nSamples;
    }

    return L;
}

vec3 Li( RayDifferential ray, Random::RNG& rng, Scene* scene )
{
    vec3 L              = vec3( 0 );
    vec3 pathThroughput = vec3( 1 );

    for ( i32 bounce = 0; bounce < scene->settings.maxDepth; ++bounce )
    {
        IntersectionData hitData;
        hitData.wo = -ray.direction;
        if ( !scene->Intersect( ray, hitData ) )
        {
            L += pathThroughput * scene->LEnvironment( ray );
            break;
        }

        hitData.position += EPSILON * hitData.normal;

        BRDF brdf = hitData.material->ComputeBRDF( &hitData );

        // emitted light of current surface
        if ( bounce == 0 && Dot( hitData.wo, hitData.normal ) > 0 )
        {
            L += pathThroughput * brdf.emissive;
        }

        // estimate direct
        vec3 Ld = LDirect( hitData, scene, rng, brdf );
        L += pathThroughput * Ld;

        // sample the BRDF to get the next ray's direction (wi)
        f32 pdf;
        vec3 wi;
        vec3 F = brdf.Sample_F( hitData.wo, wi, rng, pdf );

        if ( pdf == 0.f || F == vec3( 0 ) )
        {
            break;
        }

        pathThroughput *= F * AbsDot( wi, hitData.normal ) / pdf;
        if ( pathThroughput == vec3( 0 ) )
        {
            break;
        }

        ray = Ray( hitData.position, wi );
    }

    return L;
}

void PathTracer::Render( i32 samplesPerPixelIteration )
{
    i32 samplesPerPixel = scene->settings.numSamplesPerPixel[samplesPerPixelIteration];
    LOG( "Rendering scene at %u x %u with SPP = %d", renderedImage.width, renderedImage.height, samplesPerPixel );

    auto timeStart = Time::GetTimePoint();
    Camera& cam    = scene->camera;

    f32 halfHeight = std::tan( cam.vFov / 2 );
    f32 halfWidth  = halfHeight * cam.aspectRatio;
    vec3 UL        = cam.position + cam.GetForwardDir() + halfHeight * cam.GetUpDir() - halfWidth * cam.GetRightDir();
    vec3 dU        = cam.GetRightDir() * ( 2 * halfWidth / renderedImage.width );
    vec3 dV        = -cam.GetUpDir() * ( 2 * halfHeight / renderedImage.height );
    UL += 0.5f * ( dU + dV ); // move to center of pixel

    std::atomic<i32> renderProgress( 0 );
    i32 onePercent = static_cast<i32>( std::ceil( renderedImage.height / 100.0f ) );
    auto AAFunc    = AntiAlias::GetAlgorithm( scene->settings.antialiasMethod );

    i32 width  = static_cast<i32>( renderedImage.width );
    i32 height = static_cast<i32>( renderedImage.height );

#pragma omp parallel for schedule( dynamic )
    for ( i32 row = 0; row < height; ++row )
    {
        for ( i32 col = 0; col < width; ++col )
        {
            PG::Random::RNG rng( row * width + col );
            vec3 imagePlanePos = UL + dV * (f32)row + dU * (f32)col;

            vec3 totalColor = vec3( 0 );
            for ( i32 rayCounter = 0; rayCounter < samplesPerPixel; ++rayCounter )
            {
                vec2 pixelOffsets   = AAFunc( rayCounter, rng );
                vec3 antiAliasedPos = imagePlanePos + dU * pixelOffsets.x + dV * pixelOffsets.y;
                Ray ray             = Ray( cam.position, Normalize( antiAliasedPos - ray.position ) );
                if ( any( isnan( ray.direction ) ) )
                {
                    throw std::runtime_error( "shiit" );
                }
                totalColor += Li( ray, rng, scene );
            }

            renderedImage.SetFromFloat4( row, col, vec4( totalColor / (f32)samplesPerPixel, 1.0f ) );
        }

        i32 rowsCompleted = ++renderProgress;
        if ( rowsCompleted % onePercent == 0 )
        {
            f32 progress = rowsCompleted / (f32)renderedImage.height;
            i32 val      = (i32)( progress * 100 + 0.5f );
            i32 lpad     = (i32)( progress * PROGRESS_BAR_WIDTH + 0.5f );
            i32 rpad     = PROGRESS_BAR_WIDTH - lpad;
            printf( "\r%3d%% [%.*s%*s]", val, lpad, PROGRESS_BAR_STR, rpad, "" );
            fflush( stdout );
        }
    }

    LOG( "\nRendered scene in %.2f seconds", Time::GetTimeSince( timeStart ) / 1000 );
}

PathTracer::PathTracer( Scene* inScene )
{
    scene         = inScene;
    renderedImage = FloatImage2D( scene->settings.imageResolution.x, scene->settings.imageResolution.y, 4 );
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    if ( scene->settings.tonemapMethod == TonemapOperator::NONE )
    {
        return renderedImage.Save( filename );
    }

    FloatImage2D tonemappedImg = renderedImage.Clone();
    TonemapFunc tonemapFunc    = GetTonemapFunction( scene->settings.tonemapMethod );
    for ( u32 pixelIndex = 0; pixelIndex < renderedImage.width * renderedImage.height; ++pixelIndex )
    {
        vec3 pixel    = renderedImage.GetFloat4( pixelIndex );
        vec3 newColor = std::exp2f( scene->camera.exposure ) * pixel;
        newColor      = tonemapFunc( newColor );
        newColor      = LinearToGammaSRGB( newColor );
        newColor      = Saturate( newColor );
        tonemappedImg.SetFromFloat4( pixelIndex, vec4( newColor, 1.0f ) );
    }
    return tonemappedImg.Save( filename );
}

} // namespace PT
