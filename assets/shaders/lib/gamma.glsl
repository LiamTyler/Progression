#ifndef __GAMMA_GLSL__
#define __GAMMA_GLSL__

float LinearToGammaSRGB( float x )
{
    if ( x <= 0.0031308f )
    {
        return 12.92f * x;
    }
    else
    {
        return 1.055f * pow( x, 1.0f / 2.4f ) - 0.055f;
    }
}


vec3 LinearToGammaSRGB( in vec3 color )
{
    vec3 ret;
    ret[0] = LinearToGammaSRGB( color[0] );
    ret[1] = LinearToGammaSRGB( color[1] );
    ret[2] = LinearToGammaSRGB( color[2] );
    
    return ret;
}


float GammaSRGBToLinear( float x )
{
    if ( x <= 0.04045f )
    {
        return x / 12.92f;
    }
    else
    {
        return pow( (x + 0.055f) / 1.055f, 2.4f );
    }
}


vec3 GammaSRGBToLinear( in vec3 color )
{
    vec3 ret;
    ret[0] = GammaSRGBToLinear( color[0] );
    ret[1] = GammaSRGBToLinear( color[1] );
    ret[2] = GammaSRGBToLinear( color[2] );
    
    return ret;
}


vec3 LinearToGammaSRGBFast( in vec3 color )
{
    return pow( color, vec3( 1.0 / 2.2 ) );
}


vec3 GammaSRGBToLinearFast( in vec3 color )
{
    return pow( color, vec3( 2.2 ) );
}

#endif // #ifndef __GAMMA_GLSL__