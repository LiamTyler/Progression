#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/types/textureset.hpp"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/random.hpp"
#include "shared/serializer.hpp"

namespace PG
{

bool Material::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const MaterialCreateInfo* createInfo = (const MaterialCreateInfo*)baseInfo;
    name = createInfo->name;
    albedoTint = createInfo->albedoTint;
    metalnessTint = createInfo->metalnessTint;
    roughnessTint = createInfo->roughnessTint;
    if ( createInfo->texturesetName.empty() )
    {
        PG_ASSERT( false, "TODO! Add default textureset/images" );
    }
    else
    {
        textureset = AssetManager::Get<Textureset>( createInfo->texturesetName );
        PG_ASSERT( textureset, "Textureset '" + createInfo->texturesetName + "' not found for material '" + name + "'" );
    }

    return true;
}


bool Material::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    MaterialCreateInfo createInfo;
    serializer->Read( createInfo.name );
    serializer->Read( createInfo.albedoTint );
    serializer->Read( createInfo.metalnessTint );
    serializer->Read( createInfo.roughnessTint );
    serializer->Read( createInfo.texturesetName );
    return Load( &createInfo );
}


bool Material::FastfileSave( Serializer* serializer ) const
{
    serializer->Write( name );
    serializer->Write( albedoTint );
    serializer->Write( metalnessTint );
    serializer->Write( roughnessTint );
    std::string texturesetName;
    if ( textureset ) texturesetName = textureset->name;
    serializer->Write( texturesetName );

    return true;
}

} // namespace PG
