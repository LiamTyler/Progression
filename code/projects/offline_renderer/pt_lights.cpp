#include "pt_lights.hpp"
#include "pt_scene.hpp"
#include "shapes.hpp"

namespace PT
{

vec3 PointLight::Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const
{
    wi  = Normalize( position - it.p );
    pdf = 1;

    f32 distToLight = Length( position - it.p );

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( scene->Occluded( shadowRay, distToLight ) )
    {
        return vec3( 0 );
    }

    return Lemit / ( distToLight * distToLight );
}

vec3 DirectionalLight::Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const
{
    wi  = -direction;
    pdf = 1;

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( scene->Occluded( shadowRay, FLT_MAX ) )
    {
        return vec3( 0 );
    }

    return Lemit;
}

vec3 AreaLight::Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const
{
    SurfaceInfo surfInfo = shape->SampleWithRespectToSolidAngle( it, rng );
    wi                   = Normalize( surfInfo.position - it.p );
    pdf                  = surfInfo.pdf;
    f32 distToLight      = Length( surfInfo.position - it.p );

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( distToLight < 0.002 || scene->Occluded( shadowRay, distToLight ) )
    {
        return vec3( 0 );
    }

    return Dot( -wi, surfInfo.normal ) > 0 ? Lemit : vec3( 0 );
}

} // namespace PT
