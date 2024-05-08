#pragma once

#include "shared/math_vec.hpp"

struct Ray
{
    Ray() = default;
    Ray( const vec3& pos, const vec3& dir ) : position( pos ), direction( dir ) {}

    vec3 Evaluate( float t ) const { return position + t * direction; }

    vec3 position;
    vec3 direction;
};

struct RayDifferential : public Ray
{
    RayDifferential( const Ray& ray ) : Ray( ray ), diffX( {} ), diffY( {} ), hasDifferentials( false ) {}

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
