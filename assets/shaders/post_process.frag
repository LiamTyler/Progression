#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec2 texCoord;

layout( set = 0, binding = 0 ) uniform sampler2D originalColor;

layout( location = 0 ) out vec4 finalColor;

const float gamma = 1.0;

void main()
{
    vec3 color = texture( originalColor, texCoord ).rgb;
    finalColor.rgb = pow( color, vec3( 1.0 / gamma ) );
    finalColor.a = 1;
    //finalColor = vec4( texCoord, 0, 1 );
}