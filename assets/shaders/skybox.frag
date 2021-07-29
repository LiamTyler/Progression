#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 UV;

layout( set = 0, binding = 0 ) uniform samplerCube skybox;

layout( location = 0 ) out vec4 finalColor;

void main()
{
	// Flip Z because in opengl, +z is forward instead of -z, because it uses a left handed system
	// https://stackoverflow.com/questions/11685608/convention-of-faces-in-opengl-cubemapping
    finalColor = texture( skybox, vec3( UV.xy, -UV.z ) );
}