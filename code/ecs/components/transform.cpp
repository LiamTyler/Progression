#include "ecs/components/transform.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace PG
{

Transform::Transform( vec3 inPosition, vec3 inRotation, vec3 inScale ) : position( inPosition ), rotation( inRotation ), scale( inScale ) {}

mat4 Transform::Matrix() const
{
    mat4 model( 1 );
    model = translate( model, position );
    model = rotate( model, rotation.z, vec3( 0, 0, 1 ) );
    model = rotate( model, rotation.x, vec3( 1, 0, 0 ) );
    model = rotate( model, rotation.y, vec3( 0, 1, 0 ) );
    model = glm::scale( model, scale );

    return model;
}

vec3 Transform::TransformPoint( vec3 p ) const { return vec3( Matrix() * vec4( p, 1 ) ); }

vec3 Transform::TransformVector( vec3 v ) const { return vec3( Matrix() * vec4( v, 0 ) ); }

Ray Transform::operator*( const Ray& ray ) const
{
    mat4 matrix = Matrix();
    Ray ret;
    ret.direction = vec3( matrix * vec4( ray.direction, 0 ) );
    ret.position  = vec3( matrix * vec4( ray.position, 1 ) );

    return ret;
}

} // namespace PG
