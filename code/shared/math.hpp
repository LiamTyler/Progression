#pragma once

#include "shared/math_vec.hpp"
#include "shared/math_matrix.hpp"
#include <algorithm>


struct Ray
{
    Ray() = default;
    Ray( const vec3& pos, const vec3& dir );

    vec3 Evaluate( float t ) const;

    vec3 position;
    vec3 direction;
};

struct RayDifferential : public Ray
{
    RayDifferential( const Ray& ray ) : Ray( ray ), hasDifferentials( false ) {}

    Ray diffX;
    Ray diffY;
    bool hasDifferentials;
};

struct Interaction
{
    vec3 p;
    vec3 n;
};

inline float AbsDot( const vec3& a, const vec3& b ) { return Dot( a, b ); }