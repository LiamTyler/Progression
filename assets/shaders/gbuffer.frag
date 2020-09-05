#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "c_shared/defines.h"
#include "c_shared/structs.h"
#include "packing.h"

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out uvec4 outDiffuseAndSpecular;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec2 texCoord;
layout( location = 2 ) in mat3 TBN;

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = PG_MATERIAL_PUSH_CONSTANT_OFFSET ) MaterialConstantBufferData material;
};

#if FOO
#define A 1
#else
#define A 2
#endif

#ifndef FOOZ
#define B 1
#else
#define B 2
#endif

void main()
{
    outPosition = vec4( posInWorldSpace, 1 );

    vec3 n = normalize( TBN[2] );
    if ( material.normalMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        n.xy = texture( textures[material.normalMapIndex], texCoord ).xy;
        n.xy = 2 * n.xy - 1;
        n.z = sqrt( 1 - n.x * n.x + n.y * n.y );
        n = normalize( TBN * n );
    }
    outNormal = vec4( EncodeOctVec( n ), 0 );
    
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 diff = texture( textures[material.diffuseTexIndex], texCoord );
        if ( diff.a < 0.01 )
        {
            discard;
        }
        Kd *= diff.xyz;
    }
    outDiffuseAndSpecular = PackDiffuseAndSpecular( Kd, material.Ks );
}
