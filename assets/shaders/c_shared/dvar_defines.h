#ifndef __DVAR_DEFINES_H__
#define __DVAR_DEFINES_H__

#include "c_shared/defines.h"

// r_materialViz
#define PG_DEBUG_MTL_DISABLED 0
#define PG_DEBUG_MTL_ALBEDO 1
#define PG_DEBUG_MTL_NORMAL 2
#define PG_DEBUG_MTL_ROUGHNESS 3
#define PG_DEBUG_MTL_METALNESS 4
#define PG_DEBUG_MTL_EMISSIVE 5
#define PG_DEBUG_MTL_GEOM_NORMAL 6
#define PG_DEBUG_MTL_GEOM_TANGENT 7
#define PG_DEBUG_MTL_GEOM_BITANGENT 8
#define PG_DEBUG_MTL_COUNT 8u // the counts are inclusive

// r_lightingViz
#define PG_DEBUG_LIGHTING_DISABLED 0
#define PG_DEBUG_LIGHTING_DIRECT 1
#define PG_DEBUG_LIGHTING_INDIRECT 2
#define PG_DEBUG_LIGHTING_DIFFUSE_INDIRECT 3
#define PG_DEBUG_LIGHTING_SPECULAR_INDIRECT 4
#define PG_DEBUG_LIGHTING_EMISSIVE 5
#define PG_DEBUG_LIGHTING_COUNT 5u

// r_skyboxViz
#define PG_DEBUG_SKY_REGULAR 0
#define PG_DEBUG_SKY_IRRADIANCE 1
#define PG_DEBUG_SKY_REFLECTION_PROBE 2
#define PG_DEBUG_SKY_BLACK 3
#define PG_DEBUG_SKY_WHITE 4
#define PG_DEBUG_SKY_COUNT 4u

// r_tonemap
#define PG_TONEMAP_METHOD_DISABLED 0
#define PG_TONEMAP_METHOD_ACES 1
#define PG_TONEMAP_METHOD_UNCHARTED_2 2
#define PG_TONEMAP_METHOD_REINHARD 3
#define PG_TONEMAP_METHOD_COUNT 3u

#define PG_R_MESHLET_VIZ_BIT 0
#define PG_R_WIREFRAME_BIT 1

#if PG_SHADER_CODE
bool IsMeshletVizEnabled()
{
    return ((globals.debug_PackedDvarBools >> PG_R_MESHLET_VIZ_BIT) & 0x1) > 0;
}

bool IsWireframeEnabled()
{
    return ((globals.debug_PackedDvarBools >> PG_R_WIREFRAME_BIT) & 0x1) > 0;
}

#else // #if PG_SHADER_CODE
inline void PackMeshletVizBit( UINT& debug_PackedDvarBools, bool enabled )
{
    if ( enabled )
        debug_PackedDvarBools |= 1u << PG_R_MESHLET_VIZ_BIT;
}

inline void PackWireframeBit( UINT& debug_PackedDvarBools, bool enabled )
{
    if ( enabled )
        debug_PackedDvarBools |= 1u << PG_R_WIREFRAME_BIT;
}
#endif // #else // #if PG_SHADER_CODE

#endif // #ifndef __DVAR_DEFINES_H__