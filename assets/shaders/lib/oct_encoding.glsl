#ifndef __OCT_ENCODING_GLSL__
#define __OCT_ENCODING_GLSL__

vec3 OctToVec3( const vec2 oct )
{
    vec3 v  = vec3( oct, 1.0f - abs( oct.x ) - abs( oct.y ) );
    float t = max( -v.z, 0.0f );
    v.x += ( v.x > 0.0f ) ? -t : t;
    v.y += ( v.y > 0.0f ) ? -t : t;
    return normalize( v );
}

vec3 OctDecodeSNorm( uint packed, uint numBits )
{
    uint m  = ( 1u << numBits ) - 1u;
    int mh = ( 1 << ( numBits - 1 ) ) - 1;
    int x  = int( packed & m );
    int y  = int( ( packed >> numBits ) & m );
    vec2 e = vec2( x - mh, y - mh ) / float( mh );

    return OctToVec3( e );
}

#endif // #ifndef __OCT_ENCODING_GLSL__