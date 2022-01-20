#pragma once

#include "shared/math.hpp"
#include <memory>

namespace PT
{

struct LightIlluminationInfo
{
    float attenuation;
    float distanceToLight;
    glm::vec3 dirToLight;
    float pdf = 1;
};

class Scene;

struct Light
{
    virtual ~Light() = default;

    glm::vec3 Lemit = glm::vec3( 0 );
    int nSamples    = 1;

    virtual glm::vec3 Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const { return glm::vec3( 0 ); }
};

struct PointLight : public Light
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );

    glm::vec3 Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const override;
};

struct DirectionalLight : public Light
{
    glm::vec3 direction = glm::vec3( 0, -1, 0 );

    glm::vec3 Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const override;
};

struct Shape;
// Area lights are generated whenever the shape's material is emissive. One area light per
// shape. Only used if you want to loop directly over a list of all lights / lit surfaces.
// Otherwise during rendering, the materials emissive value is just used.
struct AreaLight : public Light
{
    std::shared_ptr< Shape > shape;

    glm::vec3 Sample_Li( const Interaction& it, glm::vec3& wi, float& pdf, Scene* scene ) const override;
};

} // namespace PT
