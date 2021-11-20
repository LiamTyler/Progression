#include "resource/resource_manager.hpp"
#include <unordered_map>

namespace PT
{

static std::unordered_map< std::string, std::shared_ptr< Material > > s_materials;
static std::unordered_map< std::string, std::shared_ptr< Model > > s_models;
static std::unordered_map< std::string, std::shared_ptr< Skybox > > s_skyboxes;
static std::unordered_map< std::string, std::shared_ptr< Texture > > s_textures;

namespace ResourceManager
{

    void Init()
    {
        s_materials.clear();
        s_models.clear();
        s_skyboxes.clear();
        s_textures.clear();
    }

    void Shutdown()
    {
        Init();
    }

    void AddMaterial( std::shared_ptr< Material > res )
    {
        assert( res->name != "" );
        s_materials[res->name] = res;
    }

    std::shared_ptr< Material > GetMaterial( const std::string& name )
    {
        auto it = s_materials.find( name );
        if ( it == s_materials.end() )
        {
            return nullptr;
        }
        return it->second;
    }

    void AddModel( std::shared_ptr< Model > res )
    {
        assert( res->name != "" );
        s_models[res->name] = res;
    }

    std::shared_ptr< Model > GetModel( const std::string& name )
    {
        auto it = s_models.find( name );
        if ( it == s_models.end() )
        {
            return nullptr;
        }
        return it->second;
    }

    void AddSkybox( std::shared_ptr< Skybox > res )
    {
        assert( res->name != "" );
        s_skyboxes[res->name] = res;
    }

    std::shared_ptr< Skybox > GetSkybox( const std::string& name )
    {
        auto it = s_skyboxes.find( name );
        if ( it == s_skyboxes.end() )
        {
            return nullptr;
        }
        return it->second;
    }

    void AddTexture( std::shared_ptr< Texture > res )
    {
        assert( res->name != "" );
        s_textures[res->name] = res;
    }

    std::shared_ptr< Texture > GetTexture( const std::string& name )
    {
        auto it = s_textures.find( name );
        if ( it == s_textures.end() )
        {
            return nullptr;
        }
        return it->second;
    }

} // namespace ResourceManager
} // namespace PT