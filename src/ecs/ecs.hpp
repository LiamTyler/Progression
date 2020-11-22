#pragma once

#include "entt/entity/registry.hpp"
#include "entt/entity/entity.hpp"

// struct lua_State;

namespace PG
{

    entt::entity GetEntityByName( entt::registry& registry, const std::string& name );

    // void RegisterLuaFunctions_ECS( lua_State* L );
    // 
    // 
    // template<typename, typename>
    // struct _ECS_export_view;
    // 
    // template< typename... Component, typename... Exclude >
    // struct _ECS_export_view< entt::type_list< Component... >, entt::type_list < Exclude... > >
    // {
    //     static entt::view< entt::exclude_t < Exclude... >, Component... > view( entt::registry &registry )
    //     {
    //         return registry.view< Component... >( entt::exclude< Exclude... > );
    //     }
    // };
    // 
    // #define REGISTER_COMPONENT_WITH_ECS( curLuaState, Comp, assignPtr ) \
    // { \
    //     using namespace entt; \
    //     auto reg_type = curLuaState["registry"].get_or_create< sol::usertype< registry > >(); \
    //     reg_type.set_function( "assign_" #Comp, assignPtr ); \
    //     reg_type.set_function( "remove_" #Comp, &registry::remove< Comp > ); \
    //     reg_type.set_function( "get_" #Comp, static_cast< Comp&( registry::* )( entity )>( &registry::get< Comp > ) ); \
    //     reg_type.set_function( "try_get_" #Comp, static_cast< Comp*( registry::*)( entity )>( &registry::try_get< Comp > ) ); \
    //     reg_type.set_function( "view_" #Comp, &_ECS_export_view< type_list< Comp >, type_list<> >::view ); \
    //     auto V = curLuaState.new_usertype< basic_view< entity, exclude_t<>, Comp > >( #Comp "_view" ); \
    //     V.set_function( "each", &basic_view< entity, exclude_t<>, Comp >::each< std::function< void( Comp& ) > > ); \
    // }

} // namespace PG
