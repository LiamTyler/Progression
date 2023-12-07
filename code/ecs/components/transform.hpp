#pragma once

#include "shared/math.hpp"

namespace PG
{

struct Transform
{
    Transform() = default;
    Transform( vec3 inPosition, vec3 inRotation, vec3 inScale );

    mat4 Matrix() const;
    vec3 TransformPoint( vec3 p ) const;
    vec3 TransformVector( vec3 v ) const;

    vec3 position = vec3( 0 );
    vec3 rotation = vec3( 0 );
    vec3 scale    = vec3( 1 );
};

} // namespace PG
