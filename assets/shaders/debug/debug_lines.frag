#version 460

#include "global_descriptors.glsl"

layout (location = 0) in vec4 lineColor;
layout (location = 0) out vec4 color;

void main()
{
    color = lineColor;
}