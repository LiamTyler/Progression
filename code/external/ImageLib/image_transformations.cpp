#include "image_transformations.hpp"
#include "shared/assert.hpp"

// +x -> right, +y -> forward, +z -> up
// uv (0, 0) is upper left corner of image
vec3 CubemapFaceUVToDirection( int cubeFace, vec2 uv )
{
    uv = 2.0f * uv - vec2( 1.0f );
    vec3 dir( 0 );
    switch ( cubeFace )
    {
    case FACE_BACK: dir = vec3( -uv.x, -1, -uv.y ); break;
    case FACE_LEFT: dir = vec3( -1, uv.x, -uv.y ); break;
    case FACE_FRONT: dir = vec3( uv.x, 1, -uv.y ); break;
    case FACE_RIGHT: dir = vec3( 1, -uv.x, -uv.y ); break;
    case FACE_TOP: dir = vec3( uv.x, uv.y, 1 ); break;
    case FACE_BOTTOM: dir = vec3( uv.x, -uv.y, -1 ); break;
    }

    return Normalize( dir );
}

// assumes direction is normalized, and right handed +Z up coordinates, upper left face coord = uv (0,0)
vec2 CubemapDirectionToFaceUV( const vec3& direction, int& faceIndex )
{
    vec3 v    = direction;
    vec3 vAbs = abs( v );
    float ma;
    vec2 uv;
    if ( vAbs.y >= vAbs.x && vAbs.y >= vAbs.z )
    {
        faceIndex = v.y < 0.0f ? FACE_BACK : FACE_FRONT;
        ma        = 0.5f / vAbs.y;
        uv        = vec2( v.y < 0.0f ? -v.x : v.x, -v.z );
    }
    else if ( vAbs.z >= vAbs.x )
    {
        faceIndex = v.z < 0.0f ? FACE_BOTTOM : FACE_TOP;
        ma        = 0.5f / vAbs.z;
        uv        = vec2( v.x, v.z < 0.0f ? -v.y : v.y );
    }
    else
    {
        faceIndex = v.x < 0.0 ? FACE_LEFT : FACE_RIGHT;
        ma        = 0.5f / vAbs.x;
        uv        = vec2( v.x < 0.0f ? v.y : -v.y, -v.z );
    }
    return uv * ma + vec2( 0.5f );
}

// http://paulbourke.net/dome/dualfish2sphere/
// NOTE: Keep in sync with skybox.frag
// atan( x, y ) instead of ( y, x ) because im assume that the center pixel of a
// lat-long image should correspond to the direction (0, +1, 0), which would be the
// 'front' face if it was a cubemap
vec2 DirectionToEquirectangularUV( const vec3& dir )
{
    // assumes normalized input dir
    float lon = atan2( dir.x, dir.y ); // -pi to pi
    float lat = acos( dir.z );         // 0 to pi
    vec2 uv   = { 0.5f * lon / PI + 0.5f, lat / PI };

    return uv;
}

vec3 EquirectangularUVToDirection( const vec2& uv )
{
    float lon = ( 2 * uv.x - 1 ) * PI; // -pi to pi
    float lat = uv.y * PI;             // 0 to pi, with 0 being the top row of the image

    vec3 dir;
    dir.x = sin( lat ) * cos( lon );
    dir.y = sin( lat ) * sin( lon );
    dir.z = cos( lat );

    return dir;
}

FloatImageCubemap EquirectangularToCubemap( const FloatImage2D& equiImg )
{
    FloatImageCubemap cubemap;
    cubemap.size        = Max( 1u, equiImg.width / 4u );
    cubemap.numChannels = equiImg.numChannels;
    for ( uint32_t i = 0; i < 6; ++i )
    {
        cubemap.faces[i] = FloatImage2D( cubemap.size, cubemap.size, cubemap.numChannels );

#pragma omp parallel for
        for ( int r = 0; r < (int)cubemap.size; ++r )
        {
            for ( int c = 0; c < (int)cubemap.size; ++c )
            {
                vec2 localUV = { ( c + 0.5f ) / (float)cubemap.size, ( r + 0.5f ) / (float)cubemap.size };
                vec3 dir     = CubemapFaceUVToDirection( i, localUV );
                vec2 equiUV  = DirectionToEquirectangularUV( dir );
                cubemap.faces[i].SetFromFloat4( r, c, equiImg.Sample( equiUV, true, true ) );
            }
        }
    }

    return cubemap;
}

FloatImage2D CubemapToEquirectangular( const FloatImageCubemap& cubemap )
{
    FloatImage2D equiImg( 4 * cubemap.size, 2 * cubemap.size, cubemap.numChannels );
    // #pragma omp parallel for
    for ( int r = 0; r < (int)equiImg.height; ++r )
    {
        for ( int c = 0; c < (int)equiImg.width; ++c )
        {
            vec2 equiUV = { ( c + 0.5f ) / (float)equiImg.width, ( r + 0.5f ) / (float)equiImg.height };
            vec3 dir    = EquirectangularUVToDirection( equiUV );
            vec2 newUV  = DirectionToEquirectangularUV( dir );
            PG_ASSERT( r == static_cast<int>( newUV.y * equiImg.height ) );
            PG_ASSERT( c == static_cast<int>( newUV.x * equiImg.width ) );
            equiImg.SetFromFloat4( r, c, cubemap.Sample( dir ) );
        }
    }

    return equiImg;
}
