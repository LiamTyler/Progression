#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/textureset.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include "shared/serializer.hpp"

namespace PG
{

bool Material::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    MaterialCreateInfo createInfo;
    serializer->Read( name );
    serializer->Read( albedoTint );
    serializer->Read( metalnessTint );
    //serializer->Read( roughnessTint );
    serializer->Read( emissiveTint );
    std::string imgName;
    serializer->Read( imgName );
    albedoMetalnessImage = AssetManager::Get<GfxImage>( imgName );
    PG_ASSERT( albedoMetalnessImage, "AlbedoMetalness image '%s' not found for material '%s'", imgName.c_str(), name.c_str() );
    serializer->Read( imgName );
    if ( !imgName.empty() )
    {
        emissiveImage = AssetManager::Get<GfxImage>( imgName );
        PG_ASSERT( emissiveImage, "Emissive image '%s' not found for material '%s'", imgName.c_str(), name.c_str() );
    }

    return true;
}


bool Material::FastfileSave( Serializer* serializer ) const
{
    serializer->Write( name );
    serializer->Write( albedoTint );
    serializer->Write( metalnessTint );
    //serializer->Write( roughnessTint );
    serializer->Write( emissiveTint );
    std::string imgName = albedoMetalnessImage ? albedoMetalnessImage->name : "";
    serializer->Write( imgName );
    imgName = emissiveImage ? emissiveImage->name : "";
    serializer->Write( imgName );

    return true;
}

} // namespace PG
