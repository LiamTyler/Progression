#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_XYZW_ONLY
#include "glm/geometric.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "shared/math_base.hpp"

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using i8vec2 = glm::vec<2, i8>;
using i8vec3 = glm::vec<3, i8>;
using i8vec4 = glm::vec<4, i8>;

using i16vec2 = glm::vec<2, i16>;
using i16vec3 = glm::vec<3, i16>;
using i16vec4 = glm::vec<4, i16>;

using ivec2 = glm::vec<2, i32>;
using ivec3 = glm::vec<3, i32>;
using ivec4 = glm::vec<4, i32>;

using u8vec2 = glm::vec<2, u8>;
using u8vec3 = glm::vec<3, u8>;
using u8vec4 = glm::vec<4, u8>;

using u16vec2 = glm::vec<2, u16>;
using u16vec3 = glm::vec<3, u16>;
using u16vec4 = glm::vec<4, u16>;

using uvec2 = glm::vec<2, u32>;
using uvec3 = glm::vec<3, u32>;
using uvec4 = glm::vec<4, u32>;

inline f32 Length( const vec2& v ) { return glm::length( v ); }
inline f32 Length( const vec3& v ) { return glm::length( v ); }
inline f32 Length( const vec4& v ) { return glm::length( v ); }

inline f32 Dot( const vec2& a, const vec2& b ) { return glm::dot( a, b ); }
inline f32 Dot( const vec3& a, const vec3& b ) { return glm::dot( a, b ); }
inline f32 Dot( const vec4& a, const vec4& b ) { return glm::dot( a, b ); }

inline f32 Cross( const vec2& a, const vec2& b ) { return a.x * b.y - a.y * b.x; }
inline vec3 Cross( const vec3& a, const vec3& b ) { return glm::cross( a, b ); }

inline vec2 Normalize( const vec2& v ) { return glm::normalize( v ); }
inline vec3 Normalize( const vec3& v ) { return glm::normalize( v ); }
inline vec4 Normalize( const vec4& v ) { return glm::normalize( v ); }

inline vec2 Saturate( const vec2& v ) { return glm::clamp( v, 0.0f, 1.0f ); }
inline vec3 Saturate( const vec3& v ) { return glm::clamp( v, 0.0f, 1.0f ); }
inline vec4 Saturate( const vec4& v ) { return glm::clamp( v, 0.0f, 1.0f ); }

inline vec2 DegToRad( const vec2& v ) { return v * ( PI / 180.0f ); }
inline vec3 DegToRad( const vec3& v ) { return v * ( PI / 180.0f ); }
inline vec4 DegToRad( const vec4& v ) { return v * ( PI / 180.0f ); }

inline vec2 RadToDeg( const vec2& v ) { return v * ( PI / 180.0f ); }
inline vec3 RadToDeg( const vec3& v ) { return v * ( PI / 180.0f ); }
inline vec4 RadToDeg( const vec4& v ) { return v * ( PI / 180.0f ); }

inline vec2 Min( const vec2& a, const vec2& b ) { return glm::min( a, b ); }
inline vec3 Min( const vec3& a, const vec3& b ) { return glm::min( a, b ); }
inline vec4 Min( const vec4& a, const vec4& b ) { return glm::min( a, b ); }

inline vec2 Max( const vec2& a, const vec2& b ) { return glm::max( a, b ); }
inline vec3 Max( const vec3& a, const vec3& b ) { return glm::max( a, b ); }
inline vec4 Max( const vec4& a, const vec4& b ) { return glm::max( a, b ); }

inline vec2 Lerp( const vec2& a, const vec2& b, f32 t ) { return glm::mix( a, b, t ); }
inline vec3 Lerp( const vec3& a, const vec3& b, f32 t ) { return glm::mix( a, b, t ); }
inline vec4 Lerp( const vec4& a, const vec4& b, f32 t ) { return glm::mix( a, b, t ); }
