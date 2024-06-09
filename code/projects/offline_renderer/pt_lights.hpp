#pragma once

#include "pt_math.hpp"
#include "shared/random.hpp"
#include <memory>

namespace PT
{

struct LightIlluminationInfo
{
    f32 attenuation;
    f32 distanceToLight;
    vec3 dirToLight;
    f32 pdf = 1;
};

class Scene;

struct Light
{
    virtual ~Light() = default;

    vec3 Lemit   = vec3( 0 );
    i32 nSamples = 1;

    virtual vec3 Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const { return vec3( 0 ); }
};

struct PointLight : public Light
{
    vec3 position = vec3( 0, 0, 0 );

    vec3 Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const override;
};

struct DirectionalLight : public Light
{
    vec3 direction = vec3( 0, -1, 0 );

    vec3 Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const override;
};

struct Shape;
// Area lights are generated whenever the shape's material is emissive. One area light per
// shape. Only used if you want to loop directly over a list of all lights / lit surfaces.
// Otherwise during rendering, the materials emissive value is just used.
struct AreaLight : public Light
{
    Shape* shape;

    vec3 Sample_Li( const Interaction& it, vec3& wi, Scene* scene, PG::Random::RNG& rng, f32& pdf ) const override;
};

} // namespace PT
