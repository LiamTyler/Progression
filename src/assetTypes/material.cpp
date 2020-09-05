#include "material.hpp"
#include "assert.hpp"
#include "asset_manager.hpp"
#include "gfx_image.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

namespace Progression
{


bool Material_Load( Material* material, const MaterialCreateInfo& createInfo )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 32, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( material );
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
    static_assert( sizeof( Material ) == sizeof( std::string ) + 32, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( material && serializer );
    MaterialCreateInfo createInfo;
    serializer->Read( createInfo.name );
    serializer->Read( createInfo.Kd );
    serializer->Read( createInfo.map_Kd_name );
    return Material_Load( material, createInfo );
}


bool Fastfile_Material_Save( const MaterialCreateInfo * const matCreateInfo, Serializer* serializer )
{
    static_assert( sizeof( Material ) == sizeof( std::string ) + 32, "Don't forget to update this function if added/removed members from Material!" );
    PG_ASSERT( matCreateInfo && serializer );
    serializer->Write( matCreateInfo->name );
    serializer->Write( matCreateInfo->Kd );
    serializer->Write( matCreateInfo->map_Kd_name );

    return true;
}


} // namespace Progression
