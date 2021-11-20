#pragma once

#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/skybox.hpp"
#include "resource/texture.hpp"
#include <memory>
#include <string>

namespace PT
{
namespace ResourceManager
{

    void Init();
    void Shutdown();

    void AddMaterial( std::shared_ptr< Material > mat );
    std::shared_ptr< Material > GetMaterial( const std::string& name );

    void AddModel( std::shared_ptr< Model > model );
    std::shared_ptr< Model > GetModel( const std::string& name );

    void AddSkybox( std::shared_ptr< Skybox > res );
    std::shared_ptr< Skybox > GetSkybox( const std::string& name );

    void AddTexture( std::shared_ptr< Texture > res );
    std::shared_ptr< Texture > GetTexture( const std::string& name );

} // namespace ResourceManager
} // namespace PT