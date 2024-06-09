#pragma once

#include "shared/math_vec.hpp"
#include "shared/platform_defines.hpp"
#if USING( GAME )

#endif // #if USING( GAME )

namespace PG
{

struct Light
{
    vec3 color    = vec3( 1 );
    f32 intensity = 1;
};

struct DirectionalLight : public Light
{
    vec3 direction = vec3( 0, 0, -1 );
};

struct PointLight : public Light
{
    vec3 position = vec3( 0, 0, 0 );
    f32 radius    = 10;
};

struct SpotLight : public Light
{
    vec3 position  = vec3( 0, 0, 0 );
    f32 radius     = 10;
    vec3 direction = vec3( 0, 0, -1 );
    f32 innerAngle = DegToRad( 20.0f );
    f32 outerAngle = DegToRad( 30.0f );
};

// #if USING( GAME )
// GpuData::PackedLight PackPointLight( const PointLight& light );
// GpuData::PackedLight PackSpotLight( const SpotLight& light );
// GpuData::PackedLight PackDirectionalLight( const DirectionalLight& light );
// #endif // #if USING( GAME )

} // namespace PG
