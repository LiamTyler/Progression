#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/mat2x2.hpp"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include <ostream>

#define PI 3.14159265358979323846f
#define PI_D 3.14159265358979323846


inline std::ostream& operator<<( std::ostream& out, const glm::vec2& v )
{
    return out << v.x << " " << v.y;
}

inline std::ostream& operator<<( std::ostream& out, const glm::vec3& v )
{
    return out << v.x << " " << v.y << " " << v.z;
}

inline std::ostream& operator<<( std::ostream& out, const glm::vec4& v )
{
    return out << v.x << " " << v.y << " " << v.z << " " << v.w;
}

inline std::ostream& operator<<( std::ostream& out, const glm::mat3& v )
{
    return out << v[0] << "\n" << v[1] << "\n" << v[2];
}

inline std::ostream& operator<<( std::ostream& out, const glm::mat4& v )
{
    return out << v[0] << "\n" << v[1] << "\n" << v[2] << "\n" << v[3];
}

inline float AbsDot( const glm::vec3& v1, const glm::vec3& v2 )
{
    return glm::abs( glm::dot( v1, v2 ) );
}

struct Ray
{
    Ray() = default;
    Ray( const glm::vec3& pos, const glm::vec3& dir );

    glm::vec3 Evaluate( float t ) const;

    glm::vec3 position;
    glm::vec3 direction;
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
    glm::vec3 p;
    glm::vec3 n;
};