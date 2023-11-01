#ifndef __PBR_GLSL__
#define __PBR_GLSL__

#define PI 3.1415926535f

vec3 PBR_FresnelSchlick( float NdotV, vec3 F0 )
{
    return F0 + (1.0 - F0) * pow( 1.0 - NdotV, 5.0 );
}


vec3 PBR_FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}


float PBR_D_GGX( float NdotH, float roughness )
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH2 = NdotH * NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float PBR_GetRemappedRoughness_Direct( float roughness )
{
    float r = roughness + 1.0f;
    return r * r / 8.0f;
}

float PBR_GetRemappedRoughness_IBL( float roughness )
{
    return roughness * roughness / 2.0f;
}

float PBR_G_SchlickGGXInternal( float NdotV, float remappedRoughness )
{
    float num   = NdotV;
    float denom = NdotV * (1.0 - remappedRoughness) + remappedRoughness;
	
    return num / denom;
}


float PBR_G_Smith( float NdotV, float NdotL, float remappedRoughness )
{
    float ggx2  = PBR_G_SchlickGGXInternal( NdotV, remappedRoughness );
    float ggx1  = PBR_G_SchlickGGXInternal( NdotL, remappedRoughness );
	
    return ggx1 * ggx2;
}


vec3 BRDF_Lambertian( vec3 albedo )
{
    return albedo / PI;
}


vec3 BRDF_CookTorrence( float D, vec3 F, float G, vec3 N, vec3 wo, vec3 wi )
{
    return (D * F * G) / (4 * dot( wo, N ) * dot( wi, N ));
}

#endif // #ifndef __PBR_GLSL__