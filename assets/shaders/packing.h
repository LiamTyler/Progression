#define PCF_HALF_WIDTH 1
#define PCF_FULL_WIDTH ( 2 * PCF_HALF_WIDTH + 1 )

// OCTAHEDRON ENCODING
vec2 signNotZero( vec2 v )
{
    return vec2( ( v.x >= 0.0 ) ? 1.0 : -1.0, ( v.y >= 0.0 ) ? 1.0 : -1.0 );
}

// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct( in vec3 v )
{
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = v.xy * ( 1.0 / ( abs( v.x ) + abs( v.y ) + abs( v.z ) ) );
    
    // Reflect the folds of the lower hemisphere over the diagonals
    return ( v.z >= 0.0 ) ? p : ( ( 1.0 - abs( p.yx ) ) * signNotZero( p ) );
}

vec3 snorm12x2_to_unorm8x3( vec2 f )
{
    // renormalize from [-1, 1] --> [0, 4094] = 12 bits
    vec2 u = vec2( round( clamp( f, -1.0, 1.0 ) * 2047 + 2047 ) );
    float t = floor( u.y / 256.0 ); // upper 4 bits of f.y
    // If storing to GL_RGB8UI, omit the final division
    return floor( vec3(
        u.x / 16.0, // most significant 8 bits of f.x 
        fract( u.x / 16.0 ) * 256.0 + t, // upper 4 bits = least significant 4 bits of u.x, lower 4 bits = most significant 4 bits of u.y
        u.y - t * 256.0 ) // least significant 8 bits of u.y
    ) / 255.0;
}

vec3 EncodeOctVec( vec3 v )
{
    return snorm12x2_to_unorm8x3( float32x3_to_oct( v ) );
}

// OCTAHEDRON DECODING
vec2 unorm8x3_to_snorm12x2( vec3 u )
{
    u *= 255.0;
    u.y *= ( 1.0 / 16.0 );
    vec2 s = vec2( u.x * 16.0 + floor( u.y ), fract( u.y ) * ( 16.0 * 256.0 ) + u.z );
    return clamp( s * ( 1.0 / 2047.0 ) - 1.0, vec2( -1.0 ), vec2( 1.0 ) );
}

vec3 oct_to_float32x3( vec2 e )
{
    vec3 v = vec3( e.xy, 1.0 - abs( e.x ) - abs( e.y ) );
    if ( v.z < 0 ) v.xy = ( 1.0 - abs( v.yx ) ) * signNotZero( v.xy );
    return normalize( v );
}

vec3 DecodeOctVec( vec3 packed )
{
    return oct_to_float32x3( unorm8x3_to_snorm12x2( packed ) );
}



uint PackTwoFloatsToShort( in const float x, in const float y )
{
    uint uX = uint( x * 0xFF );
    uint uY = uint( y * 0xFF );
    return ( uX << 8 ) | ( uY & 0xFF );
}

uvec4 PackDiffuseAndSpecular( in const vec3 Kd, in const vec4 Ks )
{
    uvec4 enc;
    enc.x = PackTwoFloatsToShort( Kd.x, Ks.x );
    enc.y = PackTwoFloatsToShort( Kd.y, Ks.y );
    enc.z = PackTwoFloatsToShort( Kd.z, Ks.z );
    enc.w = uint( Ks.w );
    
    return enc;
}

void UnpackShortToTwoFloats( in const uint packed, out float x, out float y )
{
    x = float( packed >> 8 ) / 255.0;
    y = float( packed & 0xFF ) / 255.0;
}

void UnpackDiffuseAndSpecular( in const uvec4 packed, out vec3 Kd, out vec4 Ks )
{
    UnpackShortToTwoFloats( packed.x, Kd.x, Ks.x );
    UnpackShortToTwoFloats( packed.y, Kd.y, Ks.y );
    UnpackShortToTwoFloats( packed.z, Kd.z, Ks.z );
    Ks.w = float( packed.w );
}
