#pragma once

#include "core/dvars.hpp"

namespace PG
{
extern Dvar r_tonemap;
extern Dvar r_ambientScale;

#if USING( DEVELOPMENT_BUILD )

extern Dvar r_geometryViz;
extern Dvar r_materialViz;
extern Dvar r_lightingViz;
extern Dvar r_skyboxViz;
extern Dvar r_skyboxReflectionMipLevel;
extern Dvar r_postProcessing;
extern Dvar r_meshletViz;
extern Dvar r_wireframe;
extern Dvar r_wireframeColor;
extern Dvar r_wireframeThickness;
extern Dvar r_debugInt;
extern Dvar r_debugUint;
extern Dvar r_debugFloat;
extern Dvar r_visualizeBoundingBoxes;
extern Dvar r_frustumCullingDebug;
extern Dvar r_meshletCulling_frustum;
extern Dvar r_meshletCulling_backface;

#endif // #if USING( DEVELOPMENT_BUILD )

} // namespace PG
