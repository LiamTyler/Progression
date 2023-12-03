#include "math.hpp"

Ray::Ray( const vec3& pos, const vec3& dir ) : position( pos ), direction( dir ) {}

vec3 Ray::Evaluate( float t ) const { return position + t * direction; }
