#ifndef __PBR_GLSL__
#define __PBR_GLSL__

#define PI 3.1415926535f

vec3 PBR_FresnelSchlick( float NdotV, vec3 F0 )
{
    return F0 + (1.0 - F0) * pow( 1.0 - NdotV, 5.0 );
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


float PBR_G_SchlickGGXInternal(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}


float PBR_G_Smith( float NdotV, float NdotL, float roughness)
{
    float ggx2  = PBR_G_SchlickGGXInternal( NdotV, roughness );
    float ggx1  = PBR_G_SchlickGGXInternal( NdotL, roughness );
	
    return ggx1 * ggx2;
}

#endif // #ifndef __PBR_GLSL__