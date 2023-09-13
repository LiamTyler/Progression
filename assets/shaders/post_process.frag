#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/structs.h"
#include "lib/gamma.glsl"
#include "lib/tonemap.glsl"

layout( location = 0 ) in vec2 texCoord;

layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};
layout( set = 3, binding = 0 ) uniform sampler2D originalColor;

layout( location = 0 ) out vec4 finalColor;


bool ShouldApplyPostProcessing()
{
    return globals.r_materialViz == 0;
}

void main()
{
    vec3 linearHdrColor = texture( originalColor, texCoord ).rgb;
    if ( ShouldApplyPostProcessing() )
    {
        vec3 toneMappedColor = Reinhard( linearHdrColor );
        finalColor.rgb = LinearToGammaSRGB( linearHdrColor );
    }
    else
    {
        finalColor.rgb = linearHdrColor;
    }

    finalColor.a = 1;
}