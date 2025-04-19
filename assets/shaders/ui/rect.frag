#version 450

#include "c_shared/clay_ui.h"

layout( std430, push_constant ) uniform UIElementConstantBufferUniform
{
    layout( offset = 0 ) UIRectElementData drawData;
};

layout( location = 0 ) out vec4 finalColor;

void main()
{
	finalColor = drawData.color;
}