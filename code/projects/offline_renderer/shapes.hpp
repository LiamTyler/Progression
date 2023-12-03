#pragma once

#include "asset/pt_material.hpp"
#include "asset/pt_model.hpp"
#include "core/bounding_box.hpp"
#include "ecs/components/transform.hpp"
#include "intersection_tests.hpp"
#include "pt_lights.hpp"
#include <memory>

namespace PT
{

class MeshInstance;

struct SurfaceInfo
{
    vec3 position;
    vec3 normal;
    float pdf;
};

struct Shape
{
    Shape() = default;

    virtual Material* GetMaterial() const = 0;
    virtual float Area() const            = 0;

    // samples shape uniformly. PDF is with respect to the solid angle from a reference point/normal
    // to the sampled shape position
    SurfaceInfo SampleWithRespectToSolidAngle( const Interaction& it, PG::Random::RNG& rng ) const;

    // samples the shape uniformly, with respect to the surface area
    virtual SurfaceInfo SampleWithRespectToArea( PG::Random::RNG& rng ) const = 0;
    virtual bool Intersect( const Ray& ray, IntersectionData* hitData ) const = 0;
    virtual bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const      = 0;
    virtual PG::AABB WorldSpaceAABB() const                                   = 0;
};

struct Sphere : public Shape
{
    std::shared_ptr<Material> material;
    vec3 position = vec3( 0 );
    vec3 rotation = vec3( 0 );
    float radius  = 1;

    PG::Transform worldToLocal;

    Material* GetMaterial() const override;
    float Area() const override;
    SurfaceInfo SampleWithRespectToArea( PG::Random::RNG& rng ) const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const override;
    PG::AABB WorldSpaceAABB() const override;
};

struct Triangle : public Shape
{
    MeshInstanceHandle meshHandle;
    // uint32_t firstVertIndex;
    uint32_t i0, i1, i2;

    Material* GetMaterial() const override;
    float Area() const override;
    SurfaceInfo SampleWithRespectToArea( PG::Random::RNG& rng ) const override;
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const override;
    bool TestIfHit( const Ray& ray, float maxT = FLT_MAX ) const override;
    PG::AABB WorldSpaceAABB() const override;
};

} // namespace PT
