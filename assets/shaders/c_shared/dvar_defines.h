#ifndef _SHADER_DVAR_DEFINES_H_
#define _SHADER_DVAR_DEFINES_H_

// r_materialViz
#define PG_DEBUG_MTL_DISABLED 0
#define PG_DEBUG_MTL_ALBEDO 1
#define PG_DEBUG_MTL_NORMAL 2
#define PG_DEBUG_MTL_ROUGHNESS 3
#define PG_DEBUG_MTL_METALNESS 4
#define PG_DEBUG_MTL_GEOM_NORMAL 5
#define PG_DEBUG_MTL_GEOM_TANGENT 6
#define PG_DEBUG_MTL_GEOM_BITANGENT 7
#define PG_DEBUG_MTL_COUNT 7u // the counts are inclusive

// r_lightingViz
#define PG_DEBUG_LIGHTING_DISABLED 0
#define PG_DEBUG_LIGHTING_DIRECT 1
#define PG_DEBUG_LIGHTING_INDIRECT 2
#define PG_DEBUG_LIGHTING_DIFFUSE_INDIRECT 3
#define PG_DEBUG_LIGHTING_SPECULAR_INDIRECT 4
#define PG_DEBUG_LIGHTING_COUNT 4u

// r_skyboxViz
#define PG_DEBUG_SKY_REGULAR 0
#define PG_DEBUG_SKY_IRRADIANCE 1
#define PG_DEBUG_SKY_REFLECTION_PROBE 2
#define PG_DEBUG_SKY_BLACK 3
#define PG_DEBUG_SKY_WHITE 4
#define PG_DEBUG_SKY_COUNT 4u

#define PG_TONEMAP_METHOD_DISABLED 0
#define PG_TONEMAP_METHOD_ACES 1
#define PG_TONEMAP_METHOD_UNCHARTED_2 2
#define PG_TONEMAP_METHOD_REINHARD 3
#define PG_TONEMAP_METHOD_COUNT 3u

#endif // #ifndef _SHADER_DVAR_DEFINES_H_