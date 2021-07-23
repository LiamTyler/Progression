#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 UV;

//layout( set = 0, binding = 0 ) uniform samplerCube skybox;

layout( location = 0 ) out vec4 finalColor;

void main()
{
    //finalColor = texture( skybox, UV );
	finalColor = vec4( 0.5, 0, 0, 1 );
}