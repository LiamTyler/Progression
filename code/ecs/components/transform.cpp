#include "ecs/components/transform.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace PG
{

Transform::Transform( glm::vec3 inPosition, glm::vec3 inRotation, glm::vec3 inScale )
    : position( inPosition ), rotation( inRotation ), scale( inScale )
{
}

glm::mat4 Transform::GetModelMatrix() const
{
    glm::mat4 model( 1 );
    model = glm::translate( model, position );
    model = glm::rotate( model, rotation.y, glm::vec3( 0, 1, 0 ) );
    model = glm::rotate( model, rotation.x, glm::vec3( 1, 0, 0 ) );
    model = glm::rotate( model, rotation.z, glm::vec3( 0, 0, 1 ) );
    model = glm::scale( model, scale );

    return model;
}

} // namespace PG
