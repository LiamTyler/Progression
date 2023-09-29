#pragma once

#include "renderer/r_dvars.hpp"

namespace PG
{

// NOTE! keep these in sync with c_shared/dvar_defines.h
Dvar r_materialViz(
    "r_materialViz",
    0u, 0u, 7u,
    "Debug dvar for visualizing material related values. Auto turns off post processing when > 0\n"
    "0 - Default\n"
    "1 - Albedo\n"
    "2 - Normal\n"
    "3 - Roughness\n"
    "4 - Metalness\n"
    "5 - Geometric Normal\n"
    "6 - Geometric Tangent\n"
    "7 - Geometric Bitangent\n"
);

Dvar r_lightingViz(
    "r_lightingViz",
    0u, 0u, 4u,
    "Debug dvar for visualizing lighting related values.\n"
    "0 - Default\n"
    "1 - Direct\n"
    "2 - Indirect\n"
    "3 - Diffuse Indirect\n"
    "4 - Specular Indirect\n"
);

Dvar r_skyboxViz(
    "r_skyboxViz",
    0u, 0u, 2u,
    "0 - Regular Sky\n"
    "1 - Sky Irradiance\n"
    "2 - Black\n"
    "3 - White\n"
);

Dvar r_postProcessing( "r_postProcessing", true, "Enable/disable post processing entirely" );
Dvar r_tonemap( "r_tonemap", true, "Enable/disable tonemapping" );

} // namespace PG