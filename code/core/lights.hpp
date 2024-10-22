#pragma once

#include "shared/math_vec.hpp"
#include "shared/platform_defines.hpp"
#if USING( GAME )

#endif // #if USING( GAME )

namespace PG
{

struct Light
{
    // Keep in sync with c_shared/lights.h defines (LIGHT_TYPE_POINT, etc)
    enum class Type : u8
    {
        DIRECTIONAL = 0,
        POINT       = 1,
        SPOT        = 2,

        COUNT
    };

    vec3 color    = vec3( 1 );
    f32 intensity = 1;
    Type type;
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

    f32 _padTo64;
    f32 _padTo64_1;
};

} // namespace PG
