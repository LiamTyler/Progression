#version 450

#include "global_descriptors.glsl"
DEFINE_GLOBAL_DESCRIPTORS();

#if IS_DEBUG_SHADER
#include "c_shared/dvar_defines.h"
#include "lib/debug_coloring.glsl"
#include "lib/debug_wireframe.glsl"

layout (location = 0) in PerVertexData
{
  flat uint meshletIdx;
} fragInput;

#endif // #if IS_DEBUG_SHADER

layout (location = 0) out vec4 color;

void main()
{
    color = vec4( 0, 1, 0, 1 );
    
#if IS_DEBUG_SHADER
    if ( IsMeshletVizEnabled() )
    {
        color = Debug_IndexToColorVec4( fragInput.meshletIdx );
    }
    if ( IsWireframeEnabled() )
    {
        color = ProcessStandardWireframe( color );
    }
#endif // #if IS_DEBUG_SHADER
}
