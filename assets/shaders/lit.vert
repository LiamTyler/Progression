#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inColor;

layout( location = 0 ) out vec3 vsColor;

void main()
{
    gl_Position = vec4( inPosition, 1.0 );
    vsColor = inColor;
}