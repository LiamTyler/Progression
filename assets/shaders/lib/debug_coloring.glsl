#ifndef __DEBUG_COLORING_GLSL__
#define __DEBUG_COLORING_GLSL__

#define PG_DEBUG_MAX_COLORS 10
vec3 pgDebugColors[PG_DEBUG_MAX_COLORS] =
{
    vec3(1,0,0), 
    vec3(0,1,0),
    vec3(0,0,1),
    vec3(1,1,0),
    vec3(1,0,1),
    vec3(0,1,1),
    vec3(1,0.5,0),
    vec3(0.5,1,0),
    vec3(0,0.5,1),
    vec3(1,1,1)
};

vec3 Debug_IndexToColorVec3( uint index )
{
    return pgDebugColors[index % PG_DEBUG_MAX_COLORS];
}

vec4 Debug_IndexToColorVec4( uint index )
{
    return vec4( pgDebugColors[index % PG_DEBUG_MAX_COLORS], 1 );
}

#endif // #ifndef __DEBUG_COLORING_GLSL__