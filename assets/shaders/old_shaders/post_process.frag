#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/dvar_defines.h"
#include "c_shared/structs.h"
#include "lib/gamma.glsl"
#include "lib/tonemap.glsl"

layout( location = 0 ) in vec2 texCoord;

layout( set = PG_SCENE_GLOBALS_DESCRIPTOR_SET, binding = PG_SCENE_CONSTS_BINDING_SLOT ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};
layout( set = 3, binding = 0 ) uniform sampler2D originalColor;

layout( location = 0 ) out vec4 finalColor;


bool ShouldApplyPostProcessing()
{
    return globals.r_postProcessing != 0 && globals.r_materialViz == 0;
}

bool ShouldApplyTonemapping()
{
    return globals.r_tonemap != 0;
}

void main()
{
    vec3 linearHdrColor = texture( originalColor, texCoord ).rgb;
    if ( !ShouldApplyPostProcessing() )
    {
        finalColor = vec4( linearHdrColor, 1 );
        return;
    }

    vec3 toneMappedColor = globals.cameraExposureAndSkyTint.x * linearHdrColor;
    if ( ShouldApplyTonemapping() )
    {
        switch ( globals.r_tonemap )
        {
        case PG_TONEMAP_METHOD_ACES:
            toneMappedColor = ACESFilm( toneMappedColor );
            break;
        case PG_TONEMAP_METHOD_UNCHARTED_2:
            toneMappedColor = Uncharted2Tonemap( toneMappedColor );
            break;
        case PG_TONEMAP_METHOD_REINHARD:
            toneMappedColor = Reinhard( toneMappedColor );
            break;
        case PG_TONEMAP_METHOD_DISABLED:
        default:
            toneMappedColor = clamp( toneMappedColor, vec3( 0 ), vec3( 1 ) );
            break;
        }
    }
    else
    {
        toneMappedColor = clamp( toneMappedColor, vec3( 0 ), vec3( 1 ) );
    }

    finalColor.rgb = LinearToGammaSRGB( toneMappedColor );
    finalColor.a = 1;
}