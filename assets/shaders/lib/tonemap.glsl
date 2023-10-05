#ifndef __TONEMAP_GLSL__
#define __TONEMAP_GLSL__

vec3 Reinhard( vec3 inHdrColor )
{
    return inHdrColor / (inHdrColor + vec3( 1 ));
}

vec3 Uncharted2Tonemap_Internal( vec3 x )
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap( vec3 inHdrColor )
{
    float exposureBias = 2.0f;
    vec3 currentColor = Uncharted2Tonemap_Internal( exposureBias * inHdrColor );
    
    vec3 W = vec3( 11.2f );
    vec3 whiteScale = vec3( 1.0f ) / Uncharted2Tonemap_Internal( W );
    
    return currentColor * whiteScale;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm( vec3 x )
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp( (x*(a*x+b)) / (x*(c*x+d)+e), vec3( 0 ), vec3( 1 ) );
}

#endif // #ifndef __TONEMAP_GLSL__