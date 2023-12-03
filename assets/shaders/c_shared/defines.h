#ifndef _SHADER_DEFINES_H_
#define _SHADER_DEFINES_H_

#ifdef PG_SHADER_CODE

#define PI 3.1415926535f

#define VEC2 vec2
#define VEC3 vec3
#define VEC4 vec4
#define UVEC2 uvec2
#define UVEC3 uvec3
#define UVEC4 uvec4
#define IVEC2 ivec2
#define IVEC3 ivec3
#define IVEC4 ivec4
#define MAT3 mat3
#define MAT4 mat4
#define UINT uint

#else // #ifdef PG_SHADER_CODE

// TODO: the preprocessor doesn't actually check for #if's before processing the includes yet
#include "shared/math.hpp"

#define VEC2 vec2
#define VEC3 vec3
#define VEC4 vec4
#define UVEC2 uvec2
#define UVEC3 uvec3
#define UVEC4 uvec4
#define IVEC2 ivec2
#define IVEC3 ivec3
#define IVEC4 ivec4
#define MAT3 mat3
#define MAT4 mat4
#define UINT uint32_t

#endif // #else // #ifdef PG_SHADER_CODE

#define PG_MAX_BINDLESS_TEXTURES 65534
#define PG_INVALID_TEXTURE_INDEX 65535

#endif // #ifndef _SHADER_DEFINES_H_