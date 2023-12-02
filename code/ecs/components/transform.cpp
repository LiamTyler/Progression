#include "ecs/components/transform.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace PG
{

Transform::Transform( glm::vec3 inPosition, glm::vec3 inRotation, glm::vec3 inScale )
    : position( inPosition ), rotation( inRotation ), scale( inScale )
{
}

glm::mat4 Transform::Matrix() const
{
    glm::mat4 model( 1 );
    model = glm::translate( model, position );
    model = glm::rotate( model, rotation.z, glm::vec3( 0, 0, 1 ) );
    model = glm::rotate( model, rotation.x, glm::vec3( 1, 0, 0 ) );
    model = glm::rotate( model, rotation.y, glm::vec3( 0, 1, 0 ) );
    model = glm::scale( model, scale );

    return model;
}

glm::vec3 Transform::TransformPoint( glm::vec3 p ) const { return glm::vec3( Matrix() * glm::vec4( p, 1 ) ); }

glm::vec3 Transform::TransformVector( glm::vec3 v ) const { return glm::vec3( Matrix() * glm::vec4( v, 0 ) ); }

Ray Transform::operator*( const Ray& ray ) const
{
    glm::mat4 matrix = Matrix();
    Ray ret;
    ret.direction = glm::vec3( matrix * glm::vec4( ray.direction, 0 ) );
    ret.position  = glm::vec3( matrix * glm::vec4( ray.position, 1 ) );

    return ret;
}

} // namespace PG
