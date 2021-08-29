#include "asset/types/material.hpp"
#include "core/assert.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

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
    if ( createInfo->albedoMapName.length() > 0 )
    {
        albedoMap = AssetManager::Get<GfxImage>( createInfo->albedoMapName );
        PG_ASSERT( albedoMap, "GfxImage '" + createInfo->albedoMapName + "' not found for material '" + name + "'" );
    }
    if ( createInfo->metalnessMapName.length() > 0 )
    {
        metalnessMap = AssetManager::Get<GfxImage>( createInfo->metalnessMapName );
        PG_ASSERT( metalnessMap, "GfxImage '" + createInfo->metalnessMapName + "' not found for material '" + name + "'" );
    }
    if ( createInfo->roughnessMapName.length() > 0 )
    {
        roughnessMap = AssetManager::Get<GfxImage>( createInfo->roughnessMapName );
        PG_ASSERT( roughnessMap, "GfxImage '" + createInfo->roughnessMapName + "' not found for material '" + name + "'" );
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
    serializer->Read( createInfo.albedoMapName );
    serializer->Read( createInfo.metalnessMapName );
    serializer->Read( createInfo.roughnessMapName );
    return Load( &createInfo );
}


bool Material::FastfileSave( Serializer* serializer ) const
{
    serializer->Write( name );
    serializer->Write( albedoTint );
    serializer->Write( metalnessTint );
    serializer->Write( roughnessTint );
    std::string albedoMapName, metalnessMapName, roughnessMapName;
    if ( albedoMap ) albedoMapName = albedoMap->name;
    if ( metalnessMap ) metalnessMapName = metalnessMap->name;
    if ( roughnessMap ) roughnessMapName = roughnessMap->name;
    serializer->Write( albedoMapName );
    serializer->Write( metalnessMapName );
    serializer->Write( roughnessMapName );

    return true;
}

} // namespace PG
