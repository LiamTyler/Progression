#include "ecs/component_factory.hpp"
#include "asset/asset_manager.hpp"
#include "components/entity_metadata.hpp"
#include "components/model_renderer.hpp"
#include "components/transform.hpp"
#include "ecs/ecs.hpp"
#include "entt/entt.hpp"
#include "shared/assert.hpp"
#include "shared/json_parsing.hpp"
#include <unordered_map>

namespace PG
{

// clang-format off
static void ParseEntityMetadata( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
{
    PG_ASSERT( value.IsObject() );
    EntityMetaData& d = registry.emplace< EntityMetaData >( e );
    static JSONFunctionMapper< entt::registry&, EntityMetaData& > mapping(
    {
        { "name", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
            {
                std::string name = ParseString( v );
                PG_ASSERT( !name.empty() );
                PG_ASSERT( ECS::GetEntityByName( reg, name ) == entt::null, "ECS already contains entity with name '%s'", name.c_str() );
                d.name = name;
            }
        },
        { "parent", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
            {
                std::string parentName = ParseString( v );
                d.parent = ECS::GetEntityByName( reg, parentName );
                PG_ASSERT( d.parent != entt::null, "No entity found with name '%s'", parentName.c_str() );
            }
        },
        { "isStatic", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
            {
                d.isStatic = ParseBool( v );
            }
        }
    });

    mapping.ForEachMember( value, registry, d );
}


static void ParseTransform( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
{
    Transform& t = registry.get_or_emplace< Transform >( e );
    static JSONFunctionMapper< Transform& > mapping(
    {
        { "position", []( const rapidjson::Value& v, Transform& t ) { t.position = ParseVec3( v ); } },
        { "rotation", []( const rapidjson::Value& v, Transform& t ) { t.rotation = DegToRad( ParseVec3( v ) ); } },
        { "scale",    []( const rapidjson::Value& v, Transform& t ) { t.scale    = ParseVec3( v ); } },
    });

    mapping.ForEachMember( value, t );
}


static void ParseModelRenderer( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
{   
    auto& comp = registry.emplace<ModelRenderer>( e );
    if ( !registry.has<Transform>( e ) )
    {
        registry.emplace<Transform>( e );
    }

    static JSONFunctionMapper< ModelRenderer& > mapping(
    {
        { "model", []( const rapidjson::Value& v, ModelRenderer& comp )
            {
                comp.model = AssetManager::Get< Model >( ParseString( v ) );
                PG_ASSERT( comp.model != nullptr, "Model with name '%s' not found", v.GetString() );
                //comp.materials = comp.model->originalMaterials;
            }
        },
        { "material", []( const rapidjson::Value& v, ModelRenderer& comp )
            {
                auto mat = AssetManager::Get<Material>( ParseString( v ) );
                PG_ASSERT( mat != nullptr, "Material with name '%s' not found", v.GetString() );
                PG_ASSERT( comp.model != nullptr, "Must specify model before assigning materials for it" );
                comp.materials.resize( comp.model->meshes.size() );
                for ( auto& matPtr : comp.materials )
                {
                    matPtr = mat;
                }
            }
        },
    });

    mapping.ForEachMember( value, comp );
}


void ParseComponent( const rapidjson::Value& value, entt::entity e, entt::registry& registry, const std::string& typeName )
{
    static JSONFunctionMapper< entt::entity, entt::registry& > mapping(
    {
        { "Metadata",        ParseEntityMetadata },
        { "Transform",       ParseTransform },
        { "ModelRenderer",   ParseModelRenderer },
    });

    mapping.Evaluate( typeName, value, std::move( e ), registry );
}
// clang-format on

} // namespace PG
