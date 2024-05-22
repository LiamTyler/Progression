#version 450

#include "lib/debug_coloring.glsl"

layout (location = 0) in PerVertexData
{
  flat uint meshletIdx;
} fragInput;

layout (location = 0) out vec4 color;

void main()
{
    color = Debug_IndexToColorVec4( fragInput.meshletIdx );
}
