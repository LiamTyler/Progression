#include "ecs/ecs.hpp"
//#include "core/lua.hpp"
#include "components/entity_metadata.hpp"

namespace PG
{

    entt::entity GetEntityByName( entt::registry& registry, const std::string& name )
    {
        entt::entity e = entt::null;
        registry.view< NameComponent >().each([&]( const entt::entity& entity, const NameComponent& component )
        {
            if ( name == component.name )
            {
                e = entity;
            }
        });
    
        return e;
    }

    // void RegisterLuaFunctions_ECS( lua_State* L )
    // {
    //     sol::state_view lua( L );
    //     sol::usertype< entt::registry > reg_type = lua.new_usertype< entt::registry >( "registry" );
    //     reg_type.set_function( "create", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
    //     reg_type.set_function( "destroy", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
    //     lua.set_function( "GetEntityByName", &GetEntityByName );
    // }

} // namespace PG
