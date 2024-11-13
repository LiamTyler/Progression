#include "ecs/components/transform.hpp"

namespace PG
{

Transform::Transform( vec3 inPosition, vec3 inRotation, vec3 inScale ) : position( inPosition ), rotation( inRotation ), scale( inScale ) {}

mat4 Transform::Matrix() const
{
    mat4 model( 1 );
    model = glm::translate( model, position );
    model = glm::rotate( model, rotation.z, vec3( 0, 0, 1 ) );
    model = glm::rotate( model, rotation.x, vec3( 1, 0, 0 ) );
    model = glm::rotate( model, rotation.y, vec3( 0, 1, 0 ) );
    model = glm::scale( model, scale );

    return model;
}

vec3 Transform::TransformPoint( vec3 p ) const { return vec3( Matrix() * vec4( p, 1 ) ); }

vec3 Transform::TransformVector( vec3 v ) const { return vec3( Matrix() * vec4( v, 0 ) ); }

} // namespace PG
