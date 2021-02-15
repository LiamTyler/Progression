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
    static_assert( sizeof( Material ) == sizeof( std::string ) + 56, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( baseInfo );
    const MaterialCreateInfo* createInfo = (const MaterialCreateInfo*)baseInfo;
    name = createInfo->name;
    albedoTint = createInfo->albedoTint;
    metalnessTint = createInfo->metalnessTint;
    roughnessTint = createInfo->roughnessTint;
    if ( createInfo->albedoMapName.length() > 0 )
    {
        albedoMap = AssetManager::Get< GfxImage >( createInfo->albedoMapName );
        PG_ASSERT( albedoMap, "GfxImage '" + createInfo->albedoMapName + "' not found for material '" + name + "'" );
    }
    if ( createInfo->metalnessMapName.length() > 0 )
    {
        metalnessMap = AssetManager::Get< GfxImage >( createInfo->metalnessMapName );
        PG_ASSERT( metalnessMap, "GfxImage '" + createInfo->metalnessMapName + "' not found for material '" + name + "'" );
    }
    if ( createInfo->roughnessMapName.length() > 0 )
    {
        roughnessMap = AssetManager::Get< GfxImage >( createInfo->roughnessMapName );
        PG_ASSERT( roughnessMap, "GfxImage '" + createInfo->roughnessMapName + "' not found for material '" + name + "'" );
    }

    return true;
}


bool Material::FastfileLoad( Serializer* serializer )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 56, "Don't forget to update this function if added/removed members from Material!" );
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


// Material has some special converter logic. Need the image string names from the createInfo struct,
// instead of actual image assets, since the AssetManager is empty during the converter
bool Material::FastfileSave( Serializer* serializer ) const
{
    return false;
}


bool Fastfile_Material_Save( const MaterialCreateInfo * const matCreateInfo, Serializer* serializer )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 56, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( matCreateInfo && serializer );
    serializer->Write( matCreateInfo->name );
    serializer->Write( matCreateInfo->albedoTint );
    serializer->Write( matCreateInfo->metalnessTint );
    serializer->Write( matCreateInfo->roughnessTint );
    serializer->Write( matCreateInfo->albedoMapName );
    serializer->Write( matCreateInfo->metalnessMapName );
    serializer->Write( matCreateInfo->roughnessMapName );

    return true;
}


} // namespace PG
