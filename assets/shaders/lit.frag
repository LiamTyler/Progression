#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "c_shared/limits.h"
#include "c_shared/structs.h"
#include "lib/pbr.glsl"

layout( location = 0 ) in vec3 worldSpacePos;
layout( location = 1 ) in vec3 worldSpaceNormal;
layout( location = 2 ) in vec2 texCoords;

layout( location = 0 ) out vec4 outColor;


layout( set = PG_SCENE_GLOBALS_BUFFER_SET, binding = 0 ) uniform SceneGlobalUBO
{
    SceneGlobals globals;
};
layout( set = PG_BINDLESS_TEXTURE_SET, binding = 0 ) uniform sampler2D textures[];
layout( std140, set = PG_LIGHTS_SET, binding = PG_POINT_LIGHTS_BIND_INDEX ) uniform PointLights
{
    PointLight pointLights[PG_MAX_NUM_GPU_POINT_LIGHTS];
};
layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = 128 ) MaterialData material;
};

void main()
{
	vec3 albedo = material.albedoTint.rgb;
	if ( material.albedoMapIndex != PG_INVALID_TEXTURE_INDEX )
	{
		albedo *= texture( textures[material.albedoMapIndex], texCoords ).rgb;
	}
    float metalness = material.metalness;
    float roughness = material.roughness;
    
    vec3 N = normalize( worldSpaceNormal );
    vec3 V = normalize( globals.cameraPos.xyz - worldSpacePos );

    vec3 F0 = vec3( 0.04 ); 
    F0 = mix( F0, albedo, metalness );
    
    // reflectance equation
    vec3 Lo = vec3( 0.0 );
    for(int i = 0; i < 4; ++i) 
    {
        PointLight light = pointLights[i];
        vec3 lightPosition = light.positionAndRadius.xyz;
        
        // calculate per-light radiance
        vec3 L = normalize( lightPosition - worldSpacePos );
        vec3 H = normalize( V + L );
        float distance    = length( lightPosition - worldSpacePos );
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = light.color.rgb * attenuation;
        
        // cook-torrance brdf
        float NdotL = max( 0, dot( N, L ) );
        float NdotV = max( 0, dot( N, V ) );
        float VdotH = max( 0, dot( V, H ) );
        float NdotH = max( 0, dot( N, H ) );
        float NDF = PBR_D_GGX( NdotH, roughness );
        float G   = PBR_G_Smith( NdotV, NdotL, roughness );
        vec3 F    = PBR_FresnelSchlick( VdotH, F0 );
        
        vec3 kS = F;
        vec3 kD = vec3( 1.0 ) - kS;
        kD *= 1.0 - metalness;
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL;
        vec3 specular     = numerator / max( denominator, 0.001 );  
        
        // add to outgoing radiance Lo 
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; 
    }
	
    vec3 ambient = vec3( 0.03 ) * albedo;
    vec3 color = ambient + Lo;
    outColor = vec4( color, 1 );
}