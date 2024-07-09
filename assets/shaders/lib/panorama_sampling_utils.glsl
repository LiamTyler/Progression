#ifndef __PANORAMA_SAMPLING_UTILS_GLSL__
#define __PANORAMA_SAMPLING_UTILS_GLSL__

vec2 DirectionToLatLong( in const vec3 dir )
{
    // NOTE: Keep in sync with image.cpp::DirectionToEquirectangularUV
    float lon = atan( dir.x, dir.y ); // -pi to pi
    float lat = acos( dir.z ); // 0 to PI
    vec2 uv = vec2( 0.5f * lon / PI + 0.5f, lat / PI );
    return uv;
}

#endif // #ifndef __PANORAMA_SAMPLING_UTILS_GLSL__