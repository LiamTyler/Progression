#ifndef __TONEMAP_GLSL__
#define __TONEMAP_GLSL__

vec3 Reinhard( vec3 inHdrColor )
{
    return inHdrColor / (inHdrColor + vec3( 1 ));
}


vec3 Uncharted2Tonemap_Partial( vec3 x )
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Uncharted2Tonemap( vec3 inHdrColor )
{
    float exposureBias = 2.0f;
    vec3 currentColor = Uncharted2Tonemap_Partial( exposureBias * inHdrColor );
    
    vec3 W = vec3( 11.2f );
    vec3 whiteScale = vec3( 1.0f ) / Uncharted2Tonemap_Partial( W );
    
    return currentColor * whiteScale;
}

#endif // #ifndef __TONEMAP_GLSL__