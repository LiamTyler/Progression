#pragma once

#include "image.hpp"

vec3 CubemapFaceUVToDirection( int cubeFace, vec2 uv );
vec2 CubemapDirectionToFaceUV( const vec3& direction, int& faceIndex );
vec2 DirectionToEquirectangularUV( const vec3& dir );
vec3 EquirectangularUVToDirection( const vec2& uv );

FloatImageCubemap EquirectangularToCubemap( const FloatImage2D& equiImg );
FloatImage2D CubemapToEquirectangular( const FloatImageCubemap& cubemap );
