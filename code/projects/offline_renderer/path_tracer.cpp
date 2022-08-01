#include "path_tracer.hpp"
#include "anti_aliasing.hpp"
#include "core/time.hpp"
#include "glm/ext.hpp"
#include "sampling.hpp"
#include "tonemap.hpp"
#include "shared/color_spaces.hpp"
#include "shared/core_defines.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include <algorithm>
#include <atomic>
#include <fstream>

#define PROGRESS_BAR_STR "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
#define PROGRESS_BAR_WIDTH 60
#define EPSILON 0.00001f

using namespace PG;

namespace PT
{

float Fresnel(const glm::vec3& I, const glm::vec3& N, const float &ior )
{
    // this happens when the material is reflective, but not refractive
    if ( ior == 1 )
    {
        return 1;
    }

    float cosi = std::min( 1.0f, std::max( 1.0f, glm::dot( I, N ) ) );
    float etai = 1, etat = ior;
    if ( cosi > 0 )
    {
        std::swap(etai, etat);
    }

    float sint = etai / etat * sqrtf( std::max( 0.0f, 1 - cosi * cosi ) );

    float kr;
    if ( sint >= 1 )
    {
        kr = 1;
    }
    else
    {
        float cost = sqrtf( std::max( 0.0f, 1 - sint * sint ) );
        cosi = fabsf( cosi );
        float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
        float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
        kr = (Rs * Rs + Rp * Rp) / 2;
    }

    return kr;
}

glm::vec3 Refract(const glm::vec3& I, const glm::vec3& N, const float &ior ) 
{ 
    float cosi  = std::min( 1.0f, std::max( 1.0f, glm::dot( I, N ) ) );
    float etai  = 1, etat = ior;
    glm::vec3 n = N; 
    if ( cosi < 0 )
    {
        cosi = -cosi;
    }
    else
    {
        std::swap( etai, etat );
        n = -N;
    } 
    float eta = etai / etat; 
    float k   = 1 - eta * eta * (1 - cosi * cosi); 
    return k < 0 ? glm::vec3( 0 ) : eta * I + (eta * cosi - sqrtf( k )) * n; 
} 

glm::vec3 EstimateSingleDirect( Light* light, const IntersectionData& hitData, Scene* scene, Random::RNG& rng, const BRDF& brdf )
{
    Interaction it{ hitData.position, hitData.normal };
    glm::vec3 wi;
    float lightPdf;

    // get incoming radiance, and how likely it was to sample that direction on the light
    glm::vec3 Li = light->Sample_Li( it, wi, scene, rng, lightPdf );
    if ( lightPdf == 0 )
    {
        return glm::vec3( 0 );
    }

    return brdf.F( hitData.wo, wi ) * Li * AbsDot( hitData.normal, wi ) / lightPdf;
}

glm::vec3 LDirect( const IntersectionData& hitData, Scene* scene, Random::RNG& rng, const BRDF& brdf )
{
    glm::vec3 L( 0 );
    for ( const auto& light : scene->lights )
    {
        glm::vec3 Ld( 0 );
        for ( int i = 0; i < light->nSamples; ++i )
        {
            Ld += EstimateSingleDirect( light, hitData, scene, rng, brdf );
        }
        L += Ld / (float)light->nSamples;
    }

    return L;
}


bool SolveLinearSystem2x2( const float A[2][2], const float B[2], float& x0, float& x1 )
{
    float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if ( std::abs( det ) < 1e-10f )
    {
        return false;
    }

    x0 = (A[1][1] * B[0] - A[0][1] * B[1]) / det;
    x1 = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
    
    return !std::isnan( x0 ) && !std::isnan( x1 );
}

void ComputeDifferentials( const RayDifferential& ray, IntersectionData* surf )
{
    if ( !ray.hasDifferentials )
    {
        surf->du = surf->dv = glm::vec2( 0 );
        return;
    }

    constexpr float EPS = 0.000001f;
    glm::vec3 dpdx( 0 );
    glm::vec3 dpdy( 0 );
    const glm::vec3& N = surf->normal;
    const glm::vec3& P = surf->position;

    float tx = glm::dot( N, ray.diffX.direction );
    float ty = glm::dot( N, ray.diffY.direction );
    if ( fabs( tx ) < EPS || fabs( ty ) < EPS )
    {
        surf->du = surf->dv = glm::vec2( 0 );
        return;
    }
        
    float d = glm::dot( surf->normal, surf->position );
    tx = -(glm::dot( N, ray.diffX.position ) - d) / tx;
    ty = -(glm::dot( N, ray.diffY.position ) - d) / ty;
    glm::vec3 px = ray.diffX.Evaluate( tx );
    glm::vec3 py = ray.diffY.Evaluate( ty );
    dpdx = px - P;
    dpdy = py - P;

    // 2 unknowns, 3 equations. Choose 2 dimensions least likely to be degenerate
    int dim[2];
    if ( std::abs( N.x ) > std::abs( N.y ) && std::abs( N.x ) > std::abs( N.z ) )
    {
        dim[0] = 1;
        dim[1] = 2;
    }
    else if ( std::abs( N.y ) > std::abs( N.z ) )
    {
        dim[0] = 0;
        dim[1] = 2;
    }
    else
    {
        dim[0] = 0;
        dim[1] = 1;
    }

    float A[2][2] = { { surf->dpdu[dim[0]], surf->dpdv[dim[0]] },
                      { surf->dpdu[dim[1]], surf->dpdv[dim[1]] } };
    float Bx[2] = { px[dim[0]] - P[dim[0]], px[dim[1]] - P[dim[1]] };
    float By[2] = { py[dim[0]] - P[dim[0]], py[dim[1]] - P[dim[1]] };
    if ( !SolveLinearSystem2x2( A, Bx, surf->du.x, surf->dv.x ) || !SolveLinearSystem2x2( A, By, surf->du.y, surf->dv.y ) )
    {
        surf->du = surf->dv = glm::vec2( 0 );
    }
}

glm::vec3 Li( RayDifferential ray, Random::RNG& rng, Scene* scene )
{
    glm::vec3 L              = glm::vec3( 0 );
    glm::vec3 pathThroughput = glm::vec3( 1 );
    
    for ( int bounce = 0; bounce < scene->settings.maxDepth; ++bounce )
    {
        IntersectionData hitData;
        hitData.wo = -ray.direction;
        if ( !scene->Intersect( ray, hitData ) )
        {
            L += pathThroughput * scene->LEnvironment( ray );
            break;
        }

        hitData.position += EPSILON * hitData.normal;
        ComputeDifferentials( ray, &hitData );

        // emitted light of current surface
        //if ( bounce == 0 && glm::dot( hitData.wo, hitData.normal ) > 0 )
        //{
        //    L += pathThroughput * hitData.material->Ke;
        //}

        BRDF brdf = hitData.material->ComputeBRDF( &hitData ); 

        // estimate direct
        glm::vec3 Ld = LDirect( hitData, scene, rng, brdf );
        L += pathThroughput * Ld;

        // sample the BRDF to get the next ray's direction (wi)
        float pdf;
        glm::vec3 wi;
        glm::vec3 F = brdf.Sample_F( hitData.wo, wi, rng, pdf );

        if ( pdf == 0.f || F == glm::vec3( 0 ) )
        {
            break;
        }
        
        pathThroughput *= F * AbsDot( wi, hitData.normal ) / pdf;
        if ( pathThroughput == glm::vec3( 0 ) )
        {
            break;
        }

        ray = Ray( hitData.position, wi );
    }

    return L;
}

void PathTracer::Render( int samplesPerPixelIteration )
{
    int samplesPerPixel = scene->settings.numSamplesPerPixel[samplesPerPixelIteration];
    LOG( "Rendering scene at %u x %u with SPP = %d", renderedImage.width, renderedImage.height, samplesPerPixel );

    auto timeStart = Time::GetTimePoint();
    Camera& cam = scene->camera;

    float halfHeight = std::tan( cam.vFov / 2 );
    float halfWidth  = halfHeight * cam.aspectRatio;
    glm::vec3 UL     = cam.position + cam.GetForwardDir() + halfHeight * cam.GetUpDir() - halfWidth * cam.GetRightDir();
    glm::vec3 dU     = cam.GetRightDir() * (2 * halfWidth  / renderedImage.width );
    glm::vec3 dV     = -cam.GetUpDir()   * (2 * halfHeight / renderedImage.height );
    UL              += 0.5f * (dU + dV); // move to center of pixel

    std::atomic< int > renderProgress( 0 );
    int onePercent = static_cast< int >( std::ceil( renderedImage.height / 100.0f ) );
    auto AAFunc = AntiAlias::GetAlgorithm( scene->settings.antialiasMethod );

    int width = static_cast<int>( renderedImage.width );
    int height = static_cast<int>( renderedImage.height );
    
    #pragma omp parallel for schedule( dynamic )
    for ( int row = 0; row < height; ++row )
    {
        for ( int col = 0; col < width; ++col )
        {
            PG::Random::RNG rng( row * width + col );
            glm::vec3 imagePlanePos = UL + dV * (float)row + dU * (float)col;

            glm::vec3 totalColor = glm::vec3( 0 );
            for ( int rayCounter = 0; rayCounter < samplesPerPixel; ++rayCounter )
            {
                glm::vec2 pixelOffsets = AAFunc( rayCounter, rng );
                glm::vec3 antiAliasedPos = imagePlanePos + dU * pixelOffsets.x + dV * pixelOffsets.y;
                Ray ray                  = Ray( cam.position, glm::normalize( antiAliasedPos - ray.position ) );
                totalColor              += Li( ray, rng, scene );
            }

            renderedImage.SetFromFloat4( row, col, glm::vec4( totalColor / (float)samplesPerPixel, 1.0f ) );
        }

        int rowsCompleted = ++renderProgress;
        if ( rowsCompleted % onePercent == 0 )
        {
            float progress = rowsCompleted / (float) renderedImage.height;
            int val  = (int) (progress * 100);
            int lpad = (int) (progress * PROGRESS_BAR_WIDTH);
            int rpad = PROGRESS_BAR_WIDTH - lpad;
            printf( "\r%3d%% [%.*s%*s]", val, lpad, PROGRESS_BAR_STR, rpad, "" );
            fflush( stdout );
        }
    }

    LOG( "\nRendered scene in %.2f seconds", Time::GetDuration( timeStart ) / 1000 );
}

PathTracer::PathTracer( Scene* inScene )
{
    scene = inScene;
    renderedImage = FloatImage( scene->settings.imageResolution.x, scene->settings.imageResolution.y, 4 );
}

bool PathTracer::SaveImage( const std::string& filename ) const
{
    if ( scene->settings.tonemapMethod == TonemapOperator::NONE )
    {
        return renderedImage.Save( filename );
    }
    
    FloatImage tonemappedImg = renderedImage.Clone();
    TonemapFunc tonemapFunc = GetTonemapFunction( scene->settings.tonemapMethod );
    for ( uint32_t pixelIndex = 0; pixelIndex < renderedImage.width * renderedImage.height; ++pixelIndex )
    {
        glm::vec3 pixel = renderedImage.GetFloat4( pixelIndex );
        glm::vec3 newColor = scene->camera.exposure * pixel;
        newColor = tonemapFunc( newColor );
        newColor = LinearToGammaSRGB( newColor );
        newColor = glm::clamp( newColor, glm::vec3( 0 ), glm::vec3( 1 ) );
        tonemappedImg.SetFromFloat4( pixelIndex, glm::vec4( newColor, 1.0f ) );
    }
    return tonemappedImg.Save( filename );
}

} // namespace PT
