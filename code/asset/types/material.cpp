#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"

#if USING( GAME )
#include "renderer/r_bindless_manager.hpp"
#endif // #if USING( GAME )

namespace PG
{

Material::Material()
{
#if USING( GAME )
    m_bindlessIndex = Gfx::BindlessManager::AddMaterial( this );
#endif // #if USING( GAME )
}

Material::~Material() { Free(); }

bool Material::FastfileLoad( Serializer* serializer )
{
    serializer->Read( type );
    serializer->Read( albedoTint );
    serializer->Read( metalnessTint );
    serializer->Read( roughnessTint );
    serializer->Read( emissiveTint );

    std::string imgName;
    serializer->Read( imgName );
    albedoMetalnessImage = AssetManager::Get<GfxImage>( imgName );
    PG_ASSERT( albedoMetalnessImage, "AlbedoMetalness image '%s' not found for material '%s'", imgName.c_str(), GetName() );

    serializer->Read( imgName );
    normalRoughnessImage = AssetManager::Get<GfxImage>( imgName );
    PG_ASSERT( normalRoughnessImage, "NormalRoughness image '%s' not found for material '%s'", imgName.c_str(), GetName() );

    serializer->Read( imgName );
    if ( !imgName.empty() )
    {
        emissiveImage = AssetManager::Get<GfxImage>( imgName );
        PG_ASSERT( emissiveImage, "Emissive image '%s' not found for material '%s'", imgName.c_str(), GetName() );
    }

    return true;
}

bool Material::FastfileSave( Serializer* serializer ) const
{
    serializer->Write( type );
    serializer->Write( albedoTint );
    serializer->Write( metalnessTint );
    serializer->Write( roughnessTint );
    serializer->Write( emissiveTint );
    std::string imgName = albedoMetalnessImage ? albedoMetalnessImage->GetName() : "";
    serializer->Write( imgName );
    imgName = normalRoughnessImage ? normalRoughnessImage->GetName() : "";
    serializer->Write( imgName );
    imgName = emissiveImage ? emissiveImage->GetName() : "";
    serializer->Write( imgName );

    return true;
}

void Material::Free()
{
#if USING( GAME )
    Gfx::BindlessManager::RemoveMaterial( m_bindlessIndex );
    m_bindlessIndex = PG_INVALID_MATERIAL_INDEX;
#endif // #if USING( GAME )
}

} // namespace PG
