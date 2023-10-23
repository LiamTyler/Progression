#ifndef __PACKING_GLSL__
#define __PACKING_GLSL__

vec3 Vec3ToUnorm( vec3 v )
{
    return 0.5f * (v + vec3( 1 ));
}

vec3 Vec3ToUnorm_Clamped( vec3 v )
{
    return clamp( 0.5f * (v + 1.0f), 0.0f, 0.0f );
}

vec3 UnpackNormalMapVal( vec3 v )
{
    return (v * 255 - vec3( 128 )) / 127.0f;
}

#endif // #ifndef __PACKING_GLSL__