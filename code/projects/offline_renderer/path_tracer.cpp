#include "path_tracer.hpp"
#include "anti_aliasing.hpp"
#include "core/time.hpp"
#include "glm/ext.hpp"
#include "sampling.hpp"
#include "tonemap.hpp"
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

glm::vec3 EstimateSingleDirect( Light* light, const IntersectionData& hitData, Scene* scene, const BRDF& brdf )
{
    Interaction it{ hitData.position, hitData.normal };
    glm::vec3 wi;
    float lightPdf;

    // get incoming radiance, and how likely it was to sample that direction on the light
    glm::vec3 Li = light->Sample_Li( it, wi, lightPdf, scene );
    if ( lightPdf == 0 )
    {
        return glm::vec3( 0 );
    }

    return brdf.F( hitData.wo, wi ) * Li * AbsDot( hitData.normal, wi ) / lightPdf;
}

glm::vec3 LDirect( const IntersectionData& hitData, Scene* scene, const BRDF& brdf )
{
    glm::vec3 L( 0 );
    for ( const auto& light : scene->lights )
    {
        glm::vec3 Ld( 0 );
        for ( int i = 0; i < light->nSamples; ++i )
        {
            Ld += EstimateSingleDirect( light, hitData, scene, brdf );
        }
        L += Ld / (float)light->nSamples;
    }

    return L;
}

glm::vec3 Li( const Ray& ray, Scene* scene )
{
    Ray currentRay           = ray;
    glm::vec3 L              = glm::vec3( 0 );
    glm::vec3 pathThroughput = glm::vec3( 1 );
    
    for ( int bounce = 0; bounce < scene->maxDepth; ++bounce )
    {
        IntersectionData hitData;
        hitData.wo = -currentRay.direction;
        if ( !scene->Intersect( currentRay, hitData ) )
        {
            L += pathThroughput * scene->LEnvironment( currentRay );
            break;
        }

        hitData.position += EPSILON * hitData.normal;

        // emitted light of current surface
        if ( bounce == 0 && glm::dot( hitData.wo, hitData.normal ) > 0 )
        {
            L += pathThroughput * hitData.material->Ke;
        }

        BRDF brdf = hitData.material->ComputeBRDF( &hitData ); 

        // estimate direct
        glm::vec3 Ld = LDirect( hitData, scene, brdf );
        L += pathThroughput * Ld;

        // sample the BRDF to get the next ray's direction (wi)
        float pdf;
        glm::vec3 wi;
        glm::vec3 F = brdf.Sample_F( hitData.wo, wi, pdf );

        if ( pdf == 0.f || F == glm::vec3( 0 ) )
        {
            break;
        }
        
        pathThroughput *= F * AbsDot( wi, hitData.normal ) / pdf;
        if ( pathThroughput == glm::vec3( 0 ) )
        {
            break;
        }

        currentRay = Ray( hitData.position, wi );
    }

    return L;
}

void PathTracer::Render( int samplesPerPixelIteration )
{
    int samplesPerPixel = scene->numSamplesPerPixel[samplesPerPixelIteration];
    LOG( "Rendering scene with SPP = %d", samplesPerPixel );

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
    
    #pragma omp parallel for schedule( dynamic )
    for ( int row = 0; row < renderedImage.height; ++row )
    {
        for ( int col = 0; col < renderedImage.width; ++col )
        {
            glm::vec3 imagePlanePos = UL + dV * (float)row + dU * (float)col;

            glm::vec3 totalColor = glm::vec3( 0 );
            for ( int rayCounter = 0; rayCounter < samplesPerPixel; ++rayCounter )
            {
                glm::vec3 antiAliasedPos = AntiAlias::Jitter( rayCounter, imagePlanePos, dU, dV );
                Ray ray                  = Ray( cam.position, glm::normalize( antiAliasedPos - ray.position ) );
                totalColor              += Li( ray, scene );
            }

            renderedImage.SetPixel( row, col, glm::vec4( totalColor / (float)samplesPerPixel, 1.0f );
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
    renderedImage = ImageF32( scene->imageResolution.x, scene->imageResolution.y );
}

bool PathTracer::SaveImage( const std::string& filename, bool tonemap ) const
{
    if ( tonemap )
    {
        ImageF32 tonemapped = renderedImage.Clone();
        tonemapped.ForEachPixel( [this]( glm::vec4& pixel )
            {
                glm::vec3 newColor = pixel;
                newColor = Uncharted2Tonemap( newColor, scene->camera.exposure );
                newColor = GammaCorrect( newColor, 2.2f );
                //newColor = PBRTGammaCorrect( newColor ) + glm::vec3( 1.0f / 512.0f );
                newColor = glm::clamp( newColor, glm::vec3( 0 ), glm::vec3( 1 ) );
                pixel = glm::vec4( newColor, 1.0f );
            }
        );
        return tonemapped.Save( filename );
    }
    else
    {
        return renderedImage.Save( filename );
    }
}

} // namespace PT
