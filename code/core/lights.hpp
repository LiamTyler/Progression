#pragma once

#include "shared/platform_defines.hpp"
#if USING( GAME )
#include "c_shared/structs.h"
#else // #if USING( GAME )
#include "glm/vec3.hpp"
#endif // #else // #if USING( GAME )
#include "glm/trigonometric.hpp"

namespace PG
{

struct Light
{
    glm::vec3 color = glm::vec3( 1 );
    float intensity = 1;
};

struct DirectionalLight : public Light
{
    glm::vec3 direction = glm::vec3( 0, 0, -1 );
};

struct PointLight : public Light
{
    glm::vec3 position = glm::vec3( 0, 0, 0 );
    float radius       = 10;
};

struct SpotLight : public Light
{
    glm::vec3 position     = glm::vec3( 0, 0, 0 );
    float radius           = 10;
    glm::vec3 direction    = glm::vec3( 0, 0, -1 );
    float innerCutoffAngle = glm::radians( 20.0f );
    float outerCutoffAngle = glm::radians( 30.0f );
};

#if USING( GAME )
GpuData::PackedLight PackPointLight( const PointLight& light );
GpuData::PackedLight PackSpotLight( const SpotLight& light );
GpuData::PackedLight PackDirectionalLight( const DirectionalLight& light );
#endif // #if USING( GAME )

} // namespace PG
