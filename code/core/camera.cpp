#include "core/camera.hpp"
#include "core/lua.hpp"
#include "shared/math.hpp" // before matrix_transform for #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/gtc/matrix_transform.hpp"

namespace PG
{

Camera::Camera()
{
    position    = glm::vec3( 0 );
    rotation    = glm::vec3( 0 );
    vFov        = glm::radians( 45.0f );
    aspectRatio = 16.0f / 9.0f;
    nearPlane   = 0.1f;
    farPlane    = 100.0f;
    exposure    = 0.0f;
    UpdateFrustum();
    UpdateProjectionMatrix();
}


void Camera::UpdateFrustum()
{
    UpdateOrientationVectors();
    UpdateViewMatrix();
    m_frustum.Update( vFov, nearPlane, farPlane, aspectRatio, position, m_currDir, m_currUp, m_currRight );
}


void Camera::UpdateOrientationVectors()
{
    glm::mat4 rot( 1 );
    rot         = glm::rotate( rot, rotation.z, glm::vec3( 0, 0, 1 ) );
    rot         = glm::rotate( rot, rotation.x, glm::vec3( 1, 0, 0 ) );
    m_currDir   = glm::vec3( rot * glm::vec4( 0, 1, 0, 0 ) );
    m_currUp    = glm::vec3( rot * glm::vec4( 0, 0, 1, 0 ) );
    m_currRight = glm::cross( m_currDir, m_currUp );
}


void Camera::UpdateViewMatrix()
{
    m_viewMatrix = glm::lookAt( position, position + m_currDir, m_currUp );
}


void Camera::UpdateProjectionMatrix()
{
    m_projectionMatrix = glm::perspective( vFov, aspectRatio, nearPlane, farPlane );
}

glm::mat4 Camera::GetV() const          { return m_viewMatrix; }
glm::mat4 Camera::GetP() const          { return m_projectionMatrix; }
glm::mat4 Camera::GetVP() const         { return m_projectionMatrix * m_viewMatrix; }
glm::vec3 Camera::GetForwardDir() const { return m_currDir; }
glm::vec3 Camera::GetUpDir() const      { return m_currUp; }
glm::vec3 Camera::GetRightDir() const   { return m_currRight; }
Frustum Camera::GetFrustum() const      { return m_frustum; }


void RegisterLuaFunctions_Camera( lua_State* L )
{
    sol::state_view state( L );
    sol::usertype< Camera > camera_type = state.new_usertype< Camera >( "Camera", sol::constructors< Camera() >() );
    camera_type["position"]                 = &Camera::position;
    camera_type["rotation"]                 = &Camera::rotation;
    camera_type["vfov"]                     = &Camera::vFov;
    camera_type["aspectRatio"]              = &Camera::aspectRatio;
    camera_type["nearPlane"]                = &Camera::nearPlane;
    camera_type["farPlane"]                 = &Camera::farPlane;
    camera_type["UpdateFrustum"]            = &Camera::UpdateFrustum;
    camera_type["UpdateOrientationVectors"] = &Camera::UpdateOrientationVectors;
    camera_type["UpdateViewMatrix"]         = &Camera::UpdateViewMatrix;
    camera_type["UpdateProjectionMatrix"]   = &Camera::UpdateProjectionMatrix;
    camera_type["GetV"]                     = &Camera::GetV;
    camera_type["GetP"]                     = &Camera::GetP;
    camera_type["GetVP"]                    = &Camera::GetVP;
    camera_type["GetForwardDir"]            = &Camera::GetForwardDir;
    camera_type["GetUpDir"]                 = &Camera::GetUpDir;
    camera_type["GetRightDir"]              = &Camera::GetRightDir;
    camera_type["GetFrustum"]               = &Camera::GetFrustum;
}

} // namespace Progression
