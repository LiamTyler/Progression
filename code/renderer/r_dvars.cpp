#pragma once

#include "renderer/r_dvars.hpp"
#include "shaders/c_shared/dvar_defines.h"

namespace PG
{

// NOTE! keep these in sync with c_shared/dvar_defines.h
Dvar r_materialViz(
    "r_materialViz",
    0u, 0u, PG_DEBUG_MTL_COUNT,
    "Debug dvar for visualizing material related values. Auto turns off post processing when > 0\n"
    "0 - Default\n"
    "1 - Albedo\n"
    "2 - Normal\n"
    "3 - Roughness\n"
    "4 - Metalness\n"
    "5 - Emissive\n"
    "6 - Geometric Normal\n"
    "7 - Geometric Tangent\n"
    "8 - Geometric Bitangent"
);

Dvar r_lightingViz(
    "r_lightingViz",
    0u, 0u, PG_DEBUG_LIGHTING_COUNT,
    "Debug dvar for visualizing lighting related values.\n"
    "0 - Default\n"
    "1 - Direct\n"
    "2 - Indirect\n"
    "3 - Diffuse Indirect\n"
    "4 - Specular Indirect\n"
    "5 - Emissive"
);

Dvar r_skyboxViz(
    "r_skyboxViz",
    0u, 0u, PG_DEBUG_SKY_COUNT,
    "0 - Regular Sky\n"
    "1 - Sky Irradiance\n"
    "2 - Sky Reflection (use r_skyboxReflectionMipLevel to control the mipLevel)\n"
    "3 - Black\n"
    "4 - White"
);

Dvar r_skyboxReflectionMipLevel( "r_skyboxReflectionMipLevel", 0.0f, 0.0f, 16.0f, "Control which mipLevel to visualize when using r_skyboxViz 2" );

Dvar r_postProcessing( "r_postProcessing", true, "Enable/disable post processing entirely" );

Dvar r_tonemap(
    "r_tonemap",
    1u, 0u, PG_TONEMAP_METHOD_COUNT,
    "0 - Disable\n"
    "1 - ACES\n"
    "2 - Uncharted 2\n"
    "3 - Reinhard"
);

Dvar r_debugInt( "r_debugInt", 0, INT_MIN, INT_MAX, "Just used as an optional debug value when doing shader dev" );
Dvar r_debugUint( "r_debugUint", 0u, 0u, UINT_MAX, "Just used as an optional debug value when doing shader dev" );
Dvar r_debugFloat( "r_debugFloat", 0.0f, -FLT_MAX, FLT_MAX, "Just used as an optional debug value when doing shader dev" );

} // namespace PG