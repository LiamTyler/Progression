#include "core/camera.hpp"
#include "core/lua.hpp"
#include "shared/math.hpp" // before matrix_transform for #define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace PG
{

Camera::Camera()
{
    position    = vec3( 0 );
    rotation    = vec3( 0 );
    vFov        = DegToRad( 45.0f );
    aspectRatio = 16.0f / 9.0f;
    nearPlane   = 0.1f;
    farPlane    = 100.0f;
    exposure    = 0.0f;
    Update();
}

void Camera::Update()
{
    UpdateOrientationVectors();

    m_projectionMatrix = glm::perspective( vFov, aspectRatio, nearPlane, farPlane );
    m_viewMatrix       = glm::lookAt( position, position + m_currDir, m_currUp );
    m_frustum.ExtractFromVPMatrix( GetVP() );
}

void Camera::UpdateOrientationVectors()
{
    mat4 rot( 1 );
    rot         = rotate( rot, rotation.z, vec3( 0, 0, 1 ) );
    rot         = rotate( rot, rotation.x, vec3( 1, 0, 0 ) );
    m_currDir   = vec3( rot * vec4( 0, 1, 0, 0 ) );
    m_currUp    = vec3( rot * vec4( 0, 0, 1, 0 ) );
    m_currRight = Cross( m_currDir, m_currUp );
}

mat4 Camera::GetV() const { return m_viewMatrix; }
mat4 Camera::GetP() const { return m_projectionMatrix; }
mat4 Camera::GetVP() const { return m_projectionMatrix * m_viewMatrix; }
vec3 Camera::GetForwardDir() const { return m_currDir; }
vec3 Camera::GetUpDir() const { return m_currUp; }
vec3 Camera::GetRightDir() const { return m_currRight; }
const Frustum& Camera::GetFrustum() const { return m_frustum; }

void RegisterLuaFunctions_Camera( lua_State* L )
{
    sol::state_view state( L );
    sol::usertype<Camera> camera_type       = state.new_usertype<Camera>( "Camera", sol::constructors<Camera()>() );
    camera_type["position"]                 = &Camera::position;
    camera_type["rotation"]                 = &Camera::rotation;
    camera_type["vfov"]                     = &Camera::vFov;
    camera_type["aspectRatio"]              = &Camera::aspectRatio;
    camera_type["nearPlane"]                = &Camera::nearPlane;
    camera_type["farPlane"]                 = &Camera::farPlane;
    camera_type["Update"]                   = &Camera::Update;
    camera_type["UpdateOrientationVectors"] = &Camera::UpdateOrientationVectors;
    camera_type["GetV"]                     = &Camera::GetV;
    camera_type["GetP"]                     = &Camera::GetP;
    camera_type["GetVP"]                    = &Camera::GetVP;
    camera_type["GetForwardDir"]            = &Camera::GetForwardDir;
    camera_type["GetUpDir"]                 = &Camera::GetUpDir;
    camera_type["GetRightDir"]              = &Camera::GetRightDir;
    camera_type["GetFrustum"]               = &Camera::GetFrustum;
}

} // namespace PG
