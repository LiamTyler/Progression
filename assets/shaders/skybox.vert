#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 vertex;

layout( std430, push_constant ) uniform PushConstantMatrix
{
    layout( offset = 0 ) mat4 VP;
};

layout( location = 0 ) out vec3 UV;

void main()
{
    UV = vertex;
    vec4 pos = VP * vec4( vertex, 1 );
    gl_Position = vec4( pos.xy, pos.w, pos.w );
}