#include "shapes.hpp"
#include "intersection_tests.hpp"
#include "sampling.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"

using namespace PG;

namespace PT
{

SurfaceInfo Shape::SampleWithRespectToSolidAngle( const Interaction& it ) const
{
    SurfaceInfo info = SampleWithRespectToArea();
    glm::vec3 wi     = info.position - it.p;
    if ( glm::length( wi ) == 0 )
    {
        info.pdf = 0;
    }
    else
    {
        float radiusSquared = glm::dot( wi, wi );
        wi = glm::normalize( wi );
        info.pdf *= radiusSquared / AbsDot( info.normal, -wi );
        if ( std::isinf( info.pdf ) )
        {
            info.pdf = 0.f;
        }
    }
    return info;
}

Material* Sphere::GetMaterial() const
{
    return material.get();
}

float Sphere::Area() const
{
    return 4 * PI * radius * radius;
}

SurfaceInfo Sphere::SampleWithRespectToArea() const
{
    SurfaceInfo info;
    glm::vec3 randNormal = UniformSampleSphere( Random::Rand(), Random::Rand() );
    info.position        = position + radius * randNormal;
    info.normal          = randNormal;
    info.pdf             = 1.0f / Area();
    return info;
}

bool Sphere::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t;
    Ray localRay = worldToLocal * ray;
    if ( !intersect::RaySphere( localRay.position, localRay.direction, glm::vec3( 0 ), 1, t, hitData->t ) )
    {
        return false;
    }

    hitData->t        = t;
    hitData->material = material.get();
    hitData->position = ray.Evaluate( t );
    hitData->normal   = glm::normalize( hitData->position - position );

    glm::vec3 localPos   = localRay.Evaluate( t );
    float theta          = atan2( localPos.z, localPos.x );
    float phi            = acosf( -localPos.y );
    hitData->texCoords.x = -0.5f * (theta / PI + 1); 
    hitData->texCoords.y = phi / PI;

    hitData->tangent   = glm::vec3( -sin( theta ), 0, cos( theta ) );
    hitData->bitangent = glm::cross( hitData->normal, hitData->tangent );

    return true;
}

bool Sphere::TestIfHit( const Ray& ray, float maxT ) const
{
    float t;
    return intersect::RaySphere( ray.position, ray.direction, position, radius, t, maxT );
}

AABB Sphere::WorldSpaceAABB() const
{
    const glm::vec3 extent = glm::vec3( radius );
    return AABB( position - extent, position + extent );
}


Material* Triangle::GetMaterial() const
{
    return PT::GetMaterial( GetMeshInstance( meshHandle )->material );
}

float Triangle::Area() const
{
    auto mesh = GetMeshInstance( meshHandle );
    const auto& p0 = mesh->positions[firstVertIndex + 0];
    const auto& p1 = mesh->positions[firstVertIndex + 1];
    const auto& p2 = mesh->positions[firstVertIndex + 2];
    return 0.5f * glm::length( glm::cross( p1 - p0, p2 - p0 ) );
}

SurfaceInfo Triangle::SampleWithRespectToArea() const
{
    SurfaceInfo info;
    glm::vec2 sample = UniformSampleTriangle( Random::Rand(), Random::Rand() );
    float u = sample.x;
    float v = sample.y;
    auto mesh = GetMeshInstance( meshHandle );
    auto i0 = firstVertIndex + 0;
    auto i1 = firstVertIndex + 1;
    auto i2 = firstVertIndex + 2;
    info.position   = u * mesh->positions[i0] + v * mesh->positions[i1] + ( 1 - u - v ) * mesh->positions[i2];
    info.normal     = glm::normalize( u * mesh->normals[i0] + v * mesh->normals[i1] + ( 1 - u - v ) * mesh->normals[i2] );
    info.pdf        = 1.0f / Area();
    return info;
}

bool Triangle::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    float t, u, v;
    auto mesh = GetMeshInstance( meshHandle );
    auto i0 = firstVertIndex + 0;
    auto i1 = firstVertIndex + 1;
    auto i2 = firstVertIndex + 2;
    if ( intersect::RayTriangle( ray.position, ray.direction, mesh->positions[i0], mesh->positions[i1], mesh->positions[i2], t, u, v, hitData->t ) )
    {
        hitData->t         = t;
        hitData->material  = GetMaterial();
        hitData->position  = ray.Evaluate( t );
        hitData->normal    = glm::normalize( ( 1 - u - v ) * mesh->normals[i0]  + u * mesh->normals[i1]  + v * mesh->normals[i2] );
        hitData->tangent   = glm::normalize( ( 1 - u - v ) * mesh->tangents[i0] + u * mesh->tangents[i1] + v * mesh->tangents[i2] );
        
        hitData->bitangent = glm::cross( hitData->normal, hitData->tangent );
        hitData->texCoords = ( 1 - u - v ) * mesh->uvs[i0] + u * mesh->uvs[i1] + v * mesh->uvs[i2];
        return true;
    }
    return false;
}

bool Triangle::TestIfHit( const Ray& ray, float maxT ) const
{
    float t, u, v;
    auto mesh = GetMeshInstance( meshHandle );
    auto i0 = firstVertIndex + 0;
    auto i1 = firstVertIndex + 1;
    auto i2 = firstVertIndex + 2;
    return intersect::RayTriangle( ray.position, ray.direction, mesh->positions[i0], mesh->positions[i1], mesh->positions[i2], t, u, v, maxT );
}

AABB Triangle::WorldSpaceAABB() const
{
    auto mesh = GetMeshInstance( meshHandle );
    auto i0 = firstVertIndex + 0;
    auto i1 = firstVertIndex + 1;
    auto i2 = firstVertIndex + 2;
    AABB aabb;
    aabb.Encompass( mesh->positions[i0] );
    aabb.Encompass( mesh->positions[i1] );
    aabb.Encompass( mesh->positions[i2] );
    return aabb;
}

} // namespace PT