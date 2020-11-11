#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[6] = vec2[](
    vec2(-1, 1),
    vec2(1, 1),
    vec2(1, -1),
    
    vec2(1, -1),
    vec2(-1, -1),
    vec2(-1, 1)
);

layout( location = 0 ) out vec2 texCoord;

void main()
{
    gl_Position = vec4( positions[gl_VertexIndex], 0.0, 1.0 );
    texCoord    = 0.5 * (positions[gl_VertexIndex] + vec2( 1, 1 ));
    //texCoord.y = 1 - texCoord.y;
}   