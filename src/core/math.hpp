#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat2x2.hpp"
#include "glm/mat3x3.hpp"
#include "glm/mat4x4.hpp"
#include <ostream>

#define PI 3.14159265358979323846f
#define PI_D 3.14159265358979323846

struct lua_State;
void RegisterLuaFunctions_Math( lua_State* L );

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