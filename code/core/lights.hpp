#pragma once

#include "shared/math_vec.hpp"
#include "shared/platform_defines.hpp"
#if USING( GAME )
#include "renderer/graphics_api/texture.hpp"
#endif // #if USING( GAME )
#include <string_view>

struct lua_State;

namespace PG
{

void RegisterLuaFunctions_Lights( lua_State* L );

// Keep in sync with c_shared/lights.h defines (LIGHT_TYPE_POINT, etc)
enum class LightType : u8
{
    DIRECTIONAL = 0,
    POINT       = 1,
    SPOT        = 2,

    COUNT
};

struct Light
{
    vec3 color    = vec3( 1 );
    f32 intensity = 1;
    LightType type;
    bool castsShadow        = false;
    bool active             = true;
    u16 shadowMapResolution = 2048;
#if USING( GAME )
    Gfx::Texture* shadowMapTex = nullptr;
#endif // #if USING( GAME )

    explicit Light( LightType inType ) : type( inType ) {}
    virtual ~Light();
    virtual void SetupShadowMap();

    void SetName( std::string_view inName );
    const char* GetName() const;

#if USING( DEVELOPMENT_BUILD )
protected:
    char* m_name = nullptr;
#endif // #if USING( DEVELOPMENT_BUILD )
};

struct DirectionalLight : public Light
{
    DirectionalLight();

    vec3 direction = vec3( 0, 0, -1 );
};

struct PointLight : public Light
{
    PointLight();

    vec3 position       = vec3( 0, 0, 0 );
    f32 radius          = 10;
    f32 shadowNearPlane = 0.25f;
};

struct SpotLight : public Light
{
    SpotLight();

    vec3 position       = vec3( 0, 0, 0 );
    f32 radius          = 10;
    vec3 direction      = vec3( 0, 0, -1 );
    f32 innerAngle      = DegToRad( 20.0f );
    f32 outerAngle      = DegToRad( 30.0f );
    f32 shadowNearPlane = 0.25f;
};

} // namespace PG
