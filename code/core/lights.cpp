#include "lights.hpp"
#include "core/lua.hpp"
#if USING( GAME )
#include "renderer/r_globals.hpp"
#endif // #if USING( GAME )

namespace PG
{

void RegisterLuaFunctions_Lights( lua_State* L )
{
    sol::state_view lua( L );

    sol::usertype<Light> l_type = lua.new_usertype<Light>( "Light" );
    l_type["color"]             = &Light::color;
    l_type["intensity"]         = &Light::intensity;
    l_type["type"]              = &Light::type;
    l_type["castsShadow"]       = &Light::castsShadow;
    l_type["active"]            = &Light::active;
    l_type["GetName"]           = &Light::GetName;

    sol::usertype<PointLight> pl_type = lua.new_usertype<PointLight>( "PointLight", sol::base_classes, sol::bases<Light>() );
    pl_type["position"]               = &PointLight::position;
    pl_type["radius"]                 = &PointLight::radius;
    pl_type["shadowNearPlane"]        = &PointLight::shadowNearPlane;

    sol::usertype<SpotLight> sl_type = lua.new_usertype<SpotLight>( "SpotLight", sol::base_classes, sol::bases<Light>() );
    sl_type["position"]              = &SpotLight::position;
    sl_type["radius"]                = &SpotLight::radius;
    sl_type["direction"]             = &SpotLight::direction;
    sl_type["innerAngle"]            = &SpotLight::innerAngle;
    sl_type["outerAngle"]            = &SpotLight::outerAngle;
    sl_type["shadowNearPlane"]       = &SpotLight::shadowNearPlane;
}

Light::~Light()
{
#if USING( GAME )
    if ( shadowMapTex )
    {
        shadowMapTex->Free();
        delete shadowMapTex;
    }
#endif // #if USING( GAME )

#if USING( DEVELOPMENT_BUILD )
    if ( m_name )
        free( m_name );
#endif // #if USING( DEVELOPMENT_BUILD )
}

void Light::SetupShadowMap()
{
    if ( !castsShadow )
        return;

#if USING( GAME )
    shadowMapTex = new Gfx::Texture;
    Gfx::TextureCreateInfo tInfo{};
    tInfo.format = PixelFormat::DEPTH_16_UNORM;
    tInfo.width  = shadowMapResolution;
    tInfo.height = shadowMapResolution;
    tInfo.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if ( type == LightType::POINT )
    {
        tInfo.type        = ImageType::TYPE_CUBEMAP;
        tInfo.arrayLayers = 6;
    }

#if USING( DEVELOPMENT_BUILD )
    Gfx::rg.device.NewTextureInPlace( *shadowMapTex, tInfo, GetName() ? GetName() : "" );
#else  // #if USING( DEVELOPMENT_BUILD )
    Gfx::rg.device.NewTextureInPlace( *shadowMapTex, tInfo );
#endif // #else // #if USING( DEVELOPMENT_BUILD )
#endif // #if USING( GAME )
}

void Light::SetName( std::string_view inName )
{
#if USING( DEVELOPMENT_BUILD )
    if ( m_name )
        free( m_name );
    m_name = (char*)malloc( inName.length() + 1 );
    strcpy( m_name, inName.data() );
#else  // #if USING( DEVELOPMENT_BUILD )
    PG_UNUSED( inName );
#endif // #else // #if USING( DEVELOPMENT_BUILD )
}

const char* Light::GetName() const
{
#if USING( DEVELOPMENT_BUILD )
    return m_name;
#else  // #if USING( DEVELOPMENT_BUILD )
    return nullptr;
#endif // #else // #if USING( DEVELOPMENT_BUILD )
}

DirectionalLight::DirectionalLight() : Light( LightType::DIRECTIONAL ) {}
PointLight::PointLight() : Light( LightType::POINT ) {}
SpotLight::SpotLight() : Light( LightType::SPOT ) {}

} // namespace PG
