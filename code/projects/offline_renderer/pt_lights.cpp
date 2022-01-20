#include "pt_lights.hpp"
#include "pt_scene.hpp"
#include "shapes.hpp"

namespace PT
{

glm::vec3 PointLight::Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const
{
    wi  = glm::normalize( position - it.p );
    pdf = 1;
    
    float distToLight = glm::length( position - it.p );

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( scene->Occluded( shadowRay, distToLight ) )
    {
        return glm::vec3( 0 );
    }

    return Lemit / (distToLight*distToLight);
}

glm::vec3 DirectionalLight::Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const
{
    wi  = -direction;
    pdf = 1;

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( scene->Occluded( shadowRay, FLT_MAX ) )
    {
        return glm::vec3( 0 );
    }

    return Lemit;
}

glm::vec3 AreaLight::Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const
{
    SurfaceInfo surfInfo = shape->SampleWithRespectToSolidAngle( it );
    wi                   = glm::normalize( surfInfo.position - it.p );
    pdf                  = surfInfo.pdf;
    float distToLight    = glm::length( surfInfo.position - it.p );

    IntersectionData shadowHit;
    Ray shadowRay( it.p, wi );
    if ( distToLight < 0.002 || scene->Occluded( shadowRay, distToLight ) )
    {
        return glm::vec3( 0 );
    }

    return glm::dot( -wi, surfInfo.normal ) > 0 ? Lemit : glm::vec3( 0 );
}

} // namespace PT
