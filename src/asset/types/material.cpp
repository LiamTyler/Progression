#include "asset/types/material.hpp"
#include "core/assert.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

namespace PG
{


bool Material_Load( Material* material, const MaterialCreateInfo& createInfo )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 56, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( material );
    material->name = createInfo.name;
    material->albedoTint = createInfo.albedoTint;
    material->metalnessTint = createInfo.metalnessTint;
    material->roughnessTint = createInfo.roughnessTint;
    if ( createInfo.albedoMapName.length() > 0 )
    {
        material->albedoMap = AssetManager::Get< GfxImage >( createInfo.albedoMapName );
        PG_ASSERT( material->albedoMap, "GfxImage '" + createInfo.albedoMapName + "' not found for material '" + material->name + "'" );
    }
    if ( createInfo.metalnessMapName.length() > 0 )
    {
        material->metalnessMap = AssetManager::Get< GfxImage >( createInfo.metalnessMapName );
        PG_ASSERT( material->metalnessMap, "GfxImage '" + createInfo.metalnessMapName + "' not found for material '" + material->name + "'" );
    }
    if ( createInfo.roughnessMapName.length() > 0 )
    {
        material->roughnessMap = AssetManager::Get< GfxImage >( createInfo.roughnessMapName );
        PG_ASSERT( material->roughnessMap, "GfxImage '" + createInfo.roughnessMapName + "' not found for material '" + material->name + "'" );
    }

    return true;
}


bool Fastfile_Material_Load( Material* material, Serializer* serializer )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 56, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( material && serializer );
    MaterialCreateInfo createInfo;
    serializer->Read( createInfo.name );
    serializer->Read( createInfo.albedoTint );
    serializer->Read( createInfo.metalnessTint );
    serializer->Read( createInfo.roughnessTint );
    serializer->Read( createInfo.albedoMapName );
    serializer->Read( createInfo.metalnessMapName );
    serializer->Read( createInfo.roughnessMapName );
    return Material_Load( material, createInfo );
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
