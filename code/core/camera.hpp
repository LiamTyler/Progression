#pragma once

#include "core/frustum.hpp"

struct lua_State;

namespace PG
{

class Camera
{
public:
    Camera();

    void UpdateFrustum();
    void UpdateOrientationVectors();
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    mat4 GetV() const;
    mat4 GetP() const;
    mat4 GetVP() const;
    vec3 GetForwardDir() const;
    vec3 GetUpDir() const;
    vec3 GetRightDir() const;
    Frustum GetFrustum() const;

    vec3 position;
    vec3 rotation;
    float vFov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    float exposure; // in EVs. So the radiance is multiplied by pow( 2, exposure )

protected:
    mat4 m_viewMatrix;
    mat4 m_projectionMatrix;
    vec3 m_currDir;
    vec3 m_currUp;
    vec3 m_currRight;
    Frustum m_frustum;
};

void RegisterLuaFunctions_Camera( lua_State* L );

} // namespace PG
