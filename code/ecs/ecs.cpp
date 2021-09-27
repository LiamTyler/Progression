#include "ecs/ecs.hpp"
#include "shared/assert.hpp"
#include "core/scene.hpp"
#include "core/lua.hpp"
#include "components/entity_metadata.hpp"
#include "components/transform.hpp"
#include "components/model_renderer.hpp"

namespace PG
{
namespace ECS
{
    entt::entity GetEntityByName( const entt::registry& ecs, const std::string& name )
    {
        auto view = ecs.view< const EntityMetaData >();
        for ( auto [entity, nameComp] : view.each() )
        {
            if ( nameComp.name == name )
            {
                return entity;
            }
        }
    
        return entt::null;
    }


    void RegisterLuaFunctions( lua_State* L )
    {
        sol::state_view state( L );
        sol::usertype< entt::registry > reg_type = state.new_usertype< entt::registry >( "ECS" );
        reg_type.set_function( "CreateEntity", static_cast< entt::entity( entt::registry::* )() >( &entt::registry::create ) );
        reg_type.set_function( "DestroyEntity", static_cast< void( entt::registry::* )( entt::entity ) >( &entt::registry::destroy ) );
        state["GetEntityByName"] = ECS::GetEntityByName;

        REGISTER_COMPONENT_WITH_ECS( state, Transform );
        sol::usertype< Transform > transform_type = state.new_usertype< Transform >( "Transform" );
        transform_type["position"] = &Transform::position;
        transform_type["rotation"] = &Transform::rotation;
        transform_type["scale"]    = &Transform::scale;
        transform_type["GetModelMatrix"] = &Transform::GetModelMatrix;

        REGISTER_COMPONENT_WITH_ECS( state, ModelRenderer );
        sol::usertype< ModelRenderer > modelRenderer_type = state.new_usertype< ModelRenderer >( "ModelRenderer" );
        modelRenderer_type["model"]     = &ModelRenderer::model;
        modelRenderer_type["materials"] = &ModelRenderer::materials;

        REGISTER_COMPONENT_WITH_ECS( state, EntityMetaData );
        sol::usertype< EntityMetaData > entityMetaData_type = state.new_usertype< EntityMetaData >( "EntityMetaData" );
        entityMetaData_type["name"]     = &EntityMetaData::name;
        entityMetaData_type["isStatic"] = &EntityMetaData::isStatic;
        entityMetaData_type["parent"]   = &EntityMetaData::parent;
    }

} // namespace ECS
} // namespace PG
