#ifndef __PBR_GLSL__
#define __PBR_GLSL__

#define PI 3.1415926535f

vec3 PBR_FresnelSchlick( float NdotV, vec3 F0 )
{
    return F0 + (1.0f - F0) * pow( 1.0f - NdotV, 5.0f );
}

vec3 PBR_FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    vec3 Fr = max( vec3( 1.0f - roughness ), F0 ) - F0;
    return F0 + Fr * pow( 1.0f - cosTheta, 5.0f );
}


float PBR_D_GGX( float NdotH, float roughness )
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float denom = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);

    return a2 / (PI * denom * denom);
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
    return NdotV / NdotV * (1.0f - remappedRoughness) + remappedRoughness;
}


float PBR_G_Smith( float NdotV, float NdotL, float remappedRoughness )
{
    float ggx2  = PBR_G_SchlickGGXInternal( NdotV, remappedRoughness );
    float ggx1  = PBR_G_SchlickGGXInternal( NdotL, remappedRoughness );
	
    return ggx1 * ggx2;
}


// https://google.github.io/filament/Filament.html#materialsystem/specularbrdf section 4.4.2
float V_SmithGGXCorrelated( float NoV, float NoL, float linearRoughness )
{
    float a2 = linearRoughness * linearRoughness;
    float GGXV = NoL * sqrt( NoV * NoV * (1.0f - a2) + a2 );
    float GGXL = NoV * sqrt( NoL * NoL * (1.0f - a2) + a2 );
    return 0.5f / (GGXV + GGXL);
}


vec3 BRDF_Lambertian( vec3 albedo )
{
    return albedo / PI;
}


vec3 BRDF_CookTorrence( float D, vec3 F, float G, vec3 N, vec3 wo, vec3 wi )
{
    return (D * F * G) / (4.0f * dot( wo, N ) * dot( wi, N ));
}

#endif // #ifndef __PBR_GLSL__