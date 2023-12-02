#pragma once

#include "entt/entity/entity.hpp"
#include "entt/entity/registry.hpp"

struct lua_State;

namespace PG::ECS
{

entt::entity GetEntityByName( const entt::registry& ecs, const std::string& name );

void RegisterLuaFunctions( lua_State* L );

template <typename, typename>
struct _ECS_export_view;

template <typename... Component, typename... Exclude>
struct _ECS_export_view<entt::type_list<Component...>, entt::type_list<Exclude...>>
{
    static entt::view<entt::exclude_t<Exclude...>, Component...> view( entt::registry& registry )
    {
        return registry.view<Component...>( entt::exclude<Exclude...> );
    }
};

#define REGISTER_COMPONENT_WITH_ECS( curLuaState, Comp )                                                                        \
    {                                                                                                                           \
        using namespace entt;                                                                                                   \
        auto reg_type = curLuaState["registry"].get_or_create<sol::usertype<registry>>();                                       \
        reg_type.set_function( "Create_" #Comp, static_cast<Comp& (registry::*)( const entity )>( &registry::emplace<Comp> ) ); \
        reg_type.set_function( "Remove_" #Comp, &registry::remove<Comp> );                                                      \
        reg_type.set_function( "Get_" #Comp, static_cast<Comp& (registry::*)( entity )>( &registry::get<Comp> ) );              \
        reg_type.set_function( "TryGet_" #Comp, static_cast<Comp* (registry::*)( entity )>( &registry::try_get<Comp> ) );       \
        auto p = &_ECS_export_view<type_list<Transform>, type_list<>>::view;                                                    \
        reg_type.set_function( "View_" #Comp, p );                                                                              \
    }

} // namespace PG::ECS
