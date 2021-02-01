#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "lib/gamma.glsl"
#include "lib/tonemap.glsl"

layout( location = 0 ) in vec2 texCoord;

layout( set = 0, binding = 0 ) uniform sampler2D originalColor;

layout( location = 0 ) out vec4 finalColor;

const float gamma = 1.0;

void main()
{
    vec3 linearHdrColor = texture( originalColor, texCoord ).rgb;
    vec3 toneMappedColor = Reinhard( linearHdrColor );
    finalColor.rgb = LinearToGammaSRGB( toneMappedColor );
    finalColor.a = 1;
}