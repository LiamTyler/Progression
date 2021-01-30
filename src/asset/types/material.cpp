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
    PG_ASSERT( material );
    material->type = createInfo.type;
    material->name = createInfo.name;
    material->Kd   = createInfo.Kd;
    if ( createInfo.map_Kd_name.length() > 0 )
    {
        material->map_Kd = AssetManager::Get< GfxImage >( createInfo.map_Kd_name );
        PG_ASSERT( material->map_Kd, "GfxImage '" + createInfo.map_Kd_name + "' not found for material '" + material->name + "'" );
    }

    return true;
}


bool Fastfile_Material_Load( Material* material, Serializer* serializer )
{
    PG_ASSERT( material && serializer );
    
    serializer->Read( material->name );
    serializer->Read( material->type );
    serializer->Read( material->Kd );

    std::string map_Kd_name;
    serializer->Read( map_Kd_name );
#if USING( COMPILING_CONVERTER )
    material->map_Kd_name = std::move( map_Kd_name );
#else // #if USING( COMPILING_CONVERTER )
    if ( map_Kd_name.length() > 0 )
    {
        material->map_Kd = AssetManager::Get< GfxImage >( map_Kd_name );
        PG_ASSERT( material->map_Kd, "GfxImage '" + map_Kd_name + "' not found for material '" + material->name + "'" );
    }
#endif // #else // #if USING( COMPILING_CONVERTER )

    return true;
}


bool Fastfile_Material_Save( const MaterialCreateInfo * const matCreateInfo, Serializer* serializer )
{
    PG_ASSERT( matCreateInfo && serializer );
    serializer->Write( matCreateInfo->name );
    serializer->Write( matCreateInfo->type );
    serializer->Write( matCreateInfo->Kd );
    serializer->Write( matCreateInfo->map_Kd_name );

    return true;
}


} // namespace PG
