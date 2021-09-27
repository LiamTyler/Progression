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

    glm::mat4 GetV() const;
    glm::mat4 GetP() const;
    glm::mat4 GetVP() const;
    glm::vec3 GetForwardDir() const;
    glm::vec3 GetUpDir() const;
    glm::vec3 GetRightDir() const;
    Frustum   GetFrustum() const;

    glm::vec3 position;
    glm::vec3 rotation;
    float vFov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

protected:
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::vec3 m_currDir;
    glm::vec3 m_currUp;
    glm::vec3 m_currRight;
    Frustum m_frustum;
};


void RegisterLuaFunctions_Camera( lua_State* L );

} // namespace PG
