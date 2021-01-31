#include "ecs/component_factory.hpp"
#include "asset/asset_manager.hpp"
#include "components/transform.hpp"
#include "components/entity_metadata.hpp"
#include "components/model_renderer.hpp"
#include "core/assert.hpp"
#include "ecs/ecs.hpp"
#include "utils/json_parsing.hpp"
#include <unordered_map>
#include "entt/entt.hpp"

namespace PG
{

    static void ParseEntityMetadata( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        PG_ASSERT( value.IsObject() );
        EntityMetaData& d = registry.emplace< EntityMetaData >( e );
        static JSONFunctionMapper< entt::registry&, EntityMetaData& > mapping(
        {
            { "name", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsString() );
                    std::string name = v.GetString();
                    PG_ASSERT( !name.empty() );
                    PG_ASSERT( ECS::GetEntityByName( reg, name ) == entt::null, "ECS already contains entity with name '" + name + "'" );
                    d.name = name;
                }
            },
            { "parent", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsString() );
                    std::string parentName = v.GetString();
                    d.parent = ECS::GetEntityByName( reg, parentName );
                    PG_ASSERT( d.parent != entt::null, "No entity found with name '" + parentName + "'" );
                }
            },
            { "isStatic", []( const rapidjson::Value& v, entt::registry& reg, EntityMetaData& d )
                {
                    PG_ASSERT( v.IsBool() );
                    d.isStatic = v.GetBool();
                }
            }
        });

        mapping.ForEachMember( value, registry, d );
    }


    static void ParseTransform( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {
        Transform& t = registry.emplace< Transform >( e );
        static JSONFunctionMapper< Transform& > mapping(
        {
            { "position", []( const rapidjson::Value& v, Transform& t ) { t.position = ParseVec3( v ); } },
            { "rotation", []( const rapidjson::Value& v, Transform& t ) { t.rotation = glm::radians( ParseVec3( v ) ); } },
            { "scale",    []( const rapidjson::Value& v, Transform& t ) { t.scale    = ParseVec3( v ); } },
        });

        mapping.ForEachMember( value, t );
    }


    static void ParseModelRenderer( const rapidjson::Value& value, entt::entity e, entt::registry& registry )
    {   
        auto& comp = registry.emplace< ModelRenderer >( e );
        static JSONFunctionMapper< ModelRenderer& > mapping(
        {
            { "model", []( const rapidjson::Value& v, ModelRenderer& comp )
                {
                    PG_ASSERT( v.IsString(), "Please provide a string of the model's name" );
                    comp.model = AssetManager::Get< Model >( v.GetString() );
                    PG_ASSERT( comp.model != nullptr, "Model with name '" + std::string( v.GetString() ) + "' not found" );
                    comp.materials = comp.model->originalMaterials;
                }
            },
            { "material", []( const rapidjson::Value& v, ModelRenderer& comp )
                {
                    PG_ASSERT( v.IsString(), "Please provide a string of the material's name" );
                    auto mat = AssetManager::Get< Material >( v.GetString() );
                    PG_ASSERT( mat != nullptr, "Material with name '" + std::string( v.GetString() ) + "' not found" );
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

} // namespace PG
